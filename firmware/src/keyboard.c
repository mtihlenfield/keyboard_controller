#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/irq.h"

#include "mcp4921.h"
#include "key_matrix.h"
#include "lkp_stack.h"

#define DAC_SPI_PORT spi1
#define DAC_PIN_CS 13 // GP13
#define DAC_PIN_SCK 10 // GP10
#define DAC_PIN_MOSI 11 // GP11
#define DAC_REFV 2.5 // Using a TL431 in it's default state
#define DAC_CLK_SPEED (1000 * 1000) // 1Mhz

// Amount of gain applied to the CV signal by our op-amp
// configuration.
#define CV_OPAMP_GAIN 3.2

#define GATE_OUT_PIN 5 // GP5

#define LED_PIN 25 // GP25

float cv_octave[12] = {
    0.0833, 0.1667, 0.2550, 0.3333, 0.4167,
    0.5000, 0.5833, 0.6667, 0.7500, 0.8333,
    0.9167, 1.000
};

struct keyboard_state {
    struct lkp_stack key_press_stack;
    int8_t octave_shift;
    struct mcp4921 dac;
} g_state;

static int init_cv_dac(struct mcp4921 *dac)
{
    memset(dac, 0, sizeof(struct mcp4921));

    // Setting up the pins we will use to communicate with the DAC
    dac->clk_pin = DAC_PIN_SCK; // Clock pin
    dac->cs_pin = DAC_PIN_CS; // Chip select pin
    dac->mosi_pin = DAC_PIN_MOSI; // Data out pin
    dac->spi_inst = spi1; // We'll use the spi1 peripheral

    dac->refv = DAC_REFV; // This is the reference voltage that is set by out TL431.
    dac->clock_speed = DAC_CLK_SPEED; // SPI Clock speed
    mcp4921_set_dac(dac, MCP4921_DAC_A); // Our DAC has two channels. We're only going to use channel A
    mcp4921_set_buff(dac, MCP4921_VREF_BUFFERED);
    mcp4921_set_gain(dac, MCP4921_GAIN_1X);
    mcp4921_set_shdn(dac, MCP4921_SHDN_ON);

    return mcp4921_init(dac);
}

/*
 * Brings the gate signal high if gate_state
 * is 1, or low if gate_state is 0.
 */
static inline void set_gate(uint8_t gate_state)
{
    gpio_put(GATE_OUT_PIN, gate_state);
}

/*
 * Deterines what the CV value should be for the given key
 */
static inline float key_to_cv(struct keyboard_state *state, enum key_id id)
{
    float key_octave = floor(id / 12);
    uint8_t note = (id % 12) - 1;

    return key_octave + state->octave_shift + cv_octave[note];
}

/*
 * Set gate up and use the last key that is still pressed to
 * set the CV output.
 */
static inline void play_last_note(struct keyboard_state *state)
{
    uint32_t current_note = lkp_get_last_key(&state->key_press_stack);

    if (!current_note) {
        // No keys currently pressed
        return;
    }

    set_gate(1);
    float cv = key_to_cv(state, current_note);
    mcp4921_set_output(&state->dac, cv / CV_OPAMP_GAIN);
}

void handle_key_event(key_event_t key_event) {
    uint8_t event_type = 0;
    uint32_t key_id = 0;

    key_event_unpack(key_event, &event_type, &key_id);

    if (KEY_PRESSED == event_type) {
        lkp_push_key(&g_state.key_press_stack, key_id);
        printf("Key %d pressed\n", key_id);

        // If gate is low, this won't matter
        // If gate is high, we want to retrigger it
        set_gate(0);
    } else {
        uint8_t was_last_pressed = lkp_pop_key(&g_state.key_press_stack, key_id);
        printf("Key %d released\n", key_id);
        if (was_last_pressed) {
            // If this was the last key pressed then either:
            // a) There are no other keys pressed and we want gate low
            // b) There are other keys pressed and we want to retrigger
            set_gate(0);
        } else {
            // Don't need to update gate or CV if a key
            // was released that wasn't the most recent key
            return;
        }
    }

    play_last_note(&g_state);
}

int main(void)
{
    stdio_init_all();

    printf("Starting keyboard controller!\n");

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    gpio_init(GATE_OUT_PIN);
    gpio_set_dir(GATE_OUT_PIN, GPIO_OUT);
    set_gate(0);

    memset(&g_state, 0, sizeof(struct keyboard_state));

    if (lkp_stack_init(&g_state.key_press_stack)) {
        printf("Failed to init lkp stack");
        return 1;
    }

    if (init_cv_dac(&g_state.dac)) {
        printf("Failed to init CV DAC");
        return 1;
    }

    if (km_init()) {
        printf("Failed to init key matrix");
        return 1;
    }

    multicore_launch_core1(km_loop);

    while (1) {
        while (km_event_queue_ready()) {
            key_event_t key_event = km_event_queue_pop_blocking();
            handle_key_event(key_event);
        }
    }

}

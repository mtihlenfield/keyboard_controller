#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/gpio.h"

#include "key_matrix.h"

#define SHIFT_REG_CLK_PIN 26 // GP26
#define SHIFT_REG_DATA_PIN 22 // GP22

// Total number of shift register outputs
#define SHIFT_REG_OUTPUTS 16

// NOTE rows are held high,
#define KEYBOARD_ROWS 6
#define KEYBOARD_COLS 11
#define NUM_KEYBOARD_KEYS (KEYBOARD_ROWS * KEYBOARD_COLS)

// Determines how frequently the entire key matrix is
// scanned. One col is scanned per poll, so this number
// is divided by KEYBOARD_COLS to determine how frequently
// the poll function runs
#define KEY_POLL_INTERVAL_US 2000

const uint8_t key_row_pins[KEYBOARD_ROWS] = {16, 17, 18, 19, 20, 21};

const uint8_t key_matrix[KEYBOARD_ROWS][KEYBOARD_COLS] = {
    {KEY_OCTAVE_UP, KEY_CS1, KEY_G1, KEY_CS2, KEY_G2, KEY_CS3, KEY_G3, KEY_CS4, KEY_G4, KEY_REST},
    {KEY_OCTAVE_DOWN, KEY_D1, KEY_GS1, KEY_D2, KEY_GS2, KEY_D3, KEY_GS3, KEY_D4, KEY_GS4, KEY_HOLD},
    {KEY_PLAY_PAUSE, KEY_DS1, KEY_A1, KEY_DS2, KEY_A2, KEY_DS3, KEY_A3, KEY_DS4, KEY_A4, KEY_FUNC},
    {KEY_RECORD, KEY_E1, KEY_AS1, KEY_E2, KEY_AS2, KEY_E3, KEY_AS3, KEY_E4, KEY_AS4, KEY_NONE},
    {KEY_STOP, KEY_F1, KEY_B1, KEY_F2, KEY_B2, KEY_F3, KEY_B3, KEY_F4, KEY_B4, KEY_NONE},
    {KEY_C1, KEY_FS1, KEY_C2, KEY_FS2, KEY_C3, KEY_FS3, KEY_C4, KEY_FS4, KEY_C5, KEY_NONE},
};

struct km_state {
    uint8_t key_state[KEYBOARD_ROWS * KEYBOARD_COLS];
    queue_t event_queue;
    uint8_t current_col;
    repeating_timer_t poll_timer;
} g_km_state;

static inline void clock_shift_reg(int clk_pin)
{
    sleep_us(1);
    gpio_put(SHIFT_REG_CLK_PIN, 1);
    sleep_us(1);
    gpio_put(SHIFT_REG_CLK_PIN, 0);
    sleep_us(1);
}

int km_event_queue_ready(void)
{
    return !queue_is_empty(&g_km_state.event_queue);
}

key_event_t km_event_queue_pop_blocking(void)
{
    key_event_t temp;
    queue_remove_blocking(&g_km_state.event_queue, &temp);

    return temp;
}

int km_init(void)
{
    gpio_init(SHIFT_REG_CLK_PIN);
    gpio_init(SHIFT_REG_DATA_PIN);
    gpio_set_dir(SHIFT_REG_CLK_PIN, GPIO_OUT);
    gpio_set_dir(SHIFT_REG_DATA_PIN, GPIO_OUT);

    gpio_put(SHIFT_REG_CLK_PIN, 0);

    // Set all of the shift register outputs high
    gpio_put(SHIFT_REG_DATA_PIN, 1);
    for (uint8_t i = 0; i < SHIFT_REG_OUTPUTS; i++) {
        clock_shift_reg(SHIFT_REG_CLK_PIN);
    }

    for (uint8_t i = 0; i < KEYBOARD_ROWS; i++) {
        uint8_t pin = key_row_pins[i];

        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    memset(&g_km_state, 0, sizeof(struct km_state));

    queue_init(&g_km_state.event_queue, sizeof(key_event_t), KM_EVENT_QUEUE_SIZE);

    return 0;
}

bool km_poll_keys(repeating_timer_t *timer)
{
    if (0 == g_km_state.current_col) {
        gpio_put(SHIFT_REG_DATA_PIN, 0);
    } else if (g_km_state.current_col >= KEYBOARD_COLS) {
        for (uint8_t i = 0; i < (SHIFT_REG_OUTPUTS - KEYBOARD_COLS); i++) {
            clock_shift_reg(SHIFT_REG_CLK_PIN);
        }

        g_km_state.current_col = 0;

        return true;
    } else {
        gpio_put(SHIFT_REG_DATA_PIN, 1);
    }

    clock_shift_reg(SHIFT_REG_CLK_PIN);

    const uint8_t col = g_km_state.current_col;

    for (uint8_t row = 0; row < KEYBOARD_ROWS; row++) {
        uint8_t is_pressed = !gpio_get(key_row_pins[row]);
        uint8_t key = key_matrix[row][col];
        uint8_t was_pressed = g_km_state.key_state[key];
        key_event_t key_event;

        // TODO debouncing: http://www.ganssle.com/debouncing-pt2.htm
        // https://my.eng.utah.edu/~cs5780/debouncing.pdf
        // I'm working with conductive elastomer switches on the keybed. I have not seen bouncing
        // on the keybed keys, but I think it's possible, and will almost certainly happen
        // with the buttons I plan to use for other functions
        if (is_pressed && !was_pressed) {
            // TODO without these prints the loop happens too quickly
            key_event = key_event_create(KEY_PRESSED, key);
            g_km_state.key_state[key] = 1;

            queue_add_blocking(&g_km_state.event_queue, &key_event);
        } else if (!is_pressed && was_pressed) {
            key_event = key_event_create(KEY_RELEASED, key);
            g_km_state.key_state[key] = 0;

            queue_add_blocking(&g_km_state.event_queue, &key_event);
        }
    }

    g_km_state.current_col++;

    return true;
}

void km_main(void)
{
    add_repeating_timer_us(
        KEY_POLL_INTERVAL_US / KEYBOARD_COLS,
        km_poll_keys,
        0,
        &g_km_state.poll_timer
    );
}

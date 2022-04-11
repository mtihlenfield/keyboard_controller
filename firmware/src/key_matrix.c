#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"

#include "key_matrix.h"

#define SHIFT_REG_CLK_PIN 26 // GP22
#define SHIFT_REG_DATA_PIN 22 // GP26

// NOTE rows are held high,
#define KEYBOARD_ROWS 6
#define KEYBOARD_COLS 9
#define NUM_KEYBOARD_KEYS 49

const uint8_t key_row_pins[KEYBOARD_ROWS] = {16, 17, 18, 19, 20, 21};

// Column 1 (B1) is currently only used for the first button
const uint8_t key_matrix[KEYBOARD_ROWS][KEYBOARD_COLS] = {
    {KEY_NONE, KEY_CS1, KEY_G1, KEY_CS2, KEY_G2, KEY_CS3, KEY_G3, KEY_CS4, KEY_G4},
    {KEY_NONE, KEY_D1, KEY_GS1, KEY_D2, KEY_GS2, KEY_D3, KEY_GS3, KEY_D4, KEY_GS4},
    {KEY_NONE, KEY_DS1, KEY_A1, KEY_DS2, KEY_A2, KEY_DS3, KEY_A3, KEY_DS4, KEY_A4},
    {KEY_NONE, KEY_E1, KEY_AS1, KEY_E2, KEY_AS2, KEY_E3, KEY_AS3, KEY_E4, KEY_AS4},
    {KEY_NONE, KEY_F1, KEY_B1, KEY_F2, KEY_B2, KEY_F3, KEY_B3, KEY_F4, KEY_B4},
    {KEY_C1, KEY_FS1, KEY_C2, KEY_FS2, KEY_C3, KEY_FS3, KEY_C4, KEY_FS4, KEY_C5},
};

uint8_t key_state[KEYBOARD_ROWS * KEYBOARD_COLS];

static inline void clock_shift_reg(int clk_pin)
{
    gpio_put(SHIFT_REG_CLK_PIN, 1);
    // The uC is too fast for the shift reg without this sleep.
    // TODO: Do some calculations to figure out how long we should be sleeping
    sleep_us(1);
    gpio_put(SHIFT_REG_CLK_PIN, 0);
}

int key_matrix_init(void)
{
    gpio_init(SHIFT_REG_CLK_PIN);
    gpio_init(SHIFT_REG_DATA_PIN);
    gpio_set_dir(SHIFT_REG_CLK_PIN, GPIO_OUT);
    gpio_set_dir(SHIFT_REG_DATA_PIN, GPIO_OUT);

    gpio_put(SHIFT_REG_DATA_PIN, 0);
    gpio_put(SHIFT_REG_CLK_PIN, 0);

    // Set all of the shift register outputs high
    gpio_put(SHIFT_REG_DATA_PIN, 1);
    for (uint8_t i = 0; i < 16; i++) {
        clock_shift_reg(SHIFT_REG_CLK_PIN);
    }

    for (uint8_t i = 0; i < KEYBOARD_ROWS; i++) {
        uint8_t pin = key_row_pins[i];

        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    memset(&key_state, 0, sizeof(key_state));

    return 0;
}

void key_matrix_loop(void)
{
    while (1) {
        // TODO: figure out a way to not have to use all 16 outputs
        //       currently I have to do this because the second shift
        //       register doesn't get reset at the end of the columns,
        //       so it gets out of sync
        for (uint8_t col = 0; col < 16; col++) {
            if (0 == col) {
                gpio_put(SHIFT_REG_DATA_PIN, 0);
            } else {
                gpio_put(SHIFT_REG_DATA_PIN, 1);
            }

            clock_shift_reg(SHIFT_REG_CLK_PIN);

            if (col >= KEYBOARD_COLS) {
                continue;
            }

            for (uint8_t row = 0; row < KEYBOARD_ROWS; row++) {
                uint8_t is_pressed = !gpio_get(key_row_pins[row]);
                uint8_t key = key_matrix[row][col];
                uint8_t was_pressed = key_state[key];

                // TODO debouncing: http://www.ganssle.com/debouncing-pt2.htm
                // https://my.eng.utah.edu/~cs5780/debouncing.pdf
                // I'm working with conductive elastomer switches on the keybed. Not sure if debouncing needed
                // Note that I don't see any bouncing on the scope, but I do see some odd stuff where the signal never
                // goes below ~1.8v. That stops happening when I introduce a delay. Am I not giving enought time for the
                // pin to go low?
                //
                // Could the diode drop be a problem?  Input pin never goes below 640mV. logic low is
                // 800mV and below
                if (is_pressed && !was_pressed) {
                    // TODO without these prints the loop happens too quickly
                    printf("Key pressed: %d, (N%d, B%d)\n", key, row + 1, col + 1);
                multicore_fifo_push_blocking(key_event_create(KEY_PRESSED, key));
                    key_state[key] = 1;
                } else if (!is_pressed && was_pressed) {
                    printf("Key released: %d, (N%d, B%d)\n", key, row + 1, col + 1);
                    multicore_fifo_push_blocking(key_event_create(KEY_RELEASED, key));
                    key_state[key] = 0;
                }
                // TODO without this sleep here, I see two button presses/releases for
                // every actual button press/release. Need to figure out why that is.
                // One thing I noticed: When a button is pressed the input goes low in a few hundred ns,
                // but it takes ~2.5us to get back up again. I think it may be the case that we're moving
                // too quickly for the input to go all the way back up to 3.3v (really, 2v)
                sleep_us(750);
            }
        }
    }
}

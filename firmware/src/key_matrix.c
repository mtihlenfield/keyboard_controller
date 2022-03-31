#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "key_matrix.h"

#define SHIFT_REG_CLK_PIN 27 // GP27
#define SHIFT_REG_DATA_PIN 26 // GP26

// NOTE rows are held high,
#define KEYBOARD_ROWS 6
#define KEYBOARD_COLS 9
#define NUM_KEYBOARD_KEYS 49

const uint8_t key_row_pins[KEYBOARD_ROWS] = {16, 17, 18, 19, 20, 21};

// Column 1 (B1) is currently only used for the first button
const uint8_t key_matrix[KEYBOARD_ROWS][KEYBOARD_COLS] = {
    {0, 2, 8, 14, 20, 26, 32, 38, 44},
    {0, 3, 9, 15, 21, 27, 33, 39, 45},
    {0, 4, 10, 16, 22, 28, 34, 40, 46},
    {0, 5, 11, 17, 23, 29, 35, 41, 47},
    {0, 6, 12, 18, 24, 30, 36, 42, 48},
    {1, 7, 13, 19, 25, 31, 37, 43, 49},
};

uint8_t key_state[KEYBOARD_ROWS * KEYBOARD_COLS];

static inline void clock_shift_reg(int clk_pin)
{
    gpio_put(SHIFT_REG_CLK_PIN, 1);
    // The uC is too fast for the shift reg without this sleep.
    // TODO: Do some calculations to figure out how long we should be sleeping
    sleep_us(0.1);
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

int key_matrix_loop(void)
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

            // TODO: might be able to make this faster by calling gpio_get_all()
            // and working with the result rather than multiple gpio_get calls.
            // Would need to determine if gpio_get_all doesn't just call gpio_get
            // a bunch of times
            for (uint8_t row = 0; row < KEYBOARD_ROWS; row++) {
                uint8_t is_pressed = !gpio_get(key_row_pins[row]);
                uint8_t key = key_matrix[row][col];
                uint8_t was_pressed = key_state[key];

                // TODO debouncing: http://www.ganssle.com/debouncing-pt2.htm
                if (is_pressed && !was_pressed) {
                    printf("Key pressed: %d, (N%d, B%d)\n", key, row + 1, col + 1);
                    key_state[key] = 1;
                } else if (!is_pressed && was_pressed) {
                    printf("Key released: %d, (N%d, B%d)\n", key, row + 1, col + 1);
                    key_state[key] = 0;
                }
            }
        }
    }
}

#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "mcp4921.h"
#include "key_matrix.h"

#define DAC_SPI_PORT spi1
#define DAC_PIN_CS 13 // GP13
#define DAC_PIN_SCK 10 // GP10
#define DAC_PIN_MOSI 11 // GP11
#define DAC_REFV 2.5 // Using a TL431 in it's default state
#define DAC_CLK_SPEED (1000 * 1000)

#define CV_OPAMP_GAIN 3.2

#define GATE_OUT_PIN 28 // GP28


const uint LED_PIN = 25;

struct mcp4921 mcp4921_dac;

static int init_cv_dac(void)
{
    memset(&mcp4921_dac, 0, sizeof(struct mcp4921));

    // Setting up the pints we will use to communicate with the DAC
    mcp4921_dac.clk_pin = DAC_PIN_SCK; // Clock pin
    mcp4921_dac.cs_pin = DAC_PIN_CS; // Chip select pin
    mcp4921_dac.mosi_pin = DAC_PIN_MOSI; // Data out pin
    mcp4921_dac.spi_inst = spi1; // We'll use the spi1 peripheral

    mcp4921_dac.refv = DAC_REFV; // This is the reference voltage that is set by out TL431.
    mcp4921_dac.clock_speed = DAC_CLK_SPEED; // SPI Clock speed
    mcp4921_set_dac(&mcp4921_dac, MCP4921_DAC_A); // Our DAC has two channels. We're only going to use channel A
    mcp4921_set_buff(&mcp4921_dac, MCP4921_VREF_BUFFERED);
    mcp4921_set_gain(&mcp4921_dac, MCP4921_GAIN_1X);
    mcp4921_set_shdn(&mcp4921_dac, MCP4921_SHDN_ON);

    return mcp4921_init(&mcp4921_dac);
}

static inline int set_cv_output(float volts)
{
    return mcp4921_set_output(&mcp4921_dac, volts / CV_OPAMP_GAIN);
}

int main()
{
    stdio_init_all();

    printf("Starting keyboard controller!\n");

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    if (init_cv_dac()) {
        printf("Failed to init CV DAC");
        return 1;
    }

    gpio_put(LED_PIN, 1);

    if (key_matrix_init()) {
        printf("Failed to init key matrix");
        return 1;
    }

    // key_matrix_loop();

    float scale[12] = {2.000, 2.083, 2.166, 2.250, 2.333, 2.416, 2.500, 2.583, 2.666, 2.750, 2.833, 2.916};

    while (1) {
        for (int i = 0; i < 12; i++) {
            printf("Setting CV out to %f volts\n", scale[i]);
            set_cv_output(scale[i]);
            sleep_ms(500);
        }
    }
}

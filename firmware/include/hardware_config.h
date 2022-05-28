#ifndef __HARDWARE_CONFIG_H__
#define __HARDWARE_CONFIG_H__

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

#define SHIFT_REG_CLK_PIN 14 // GP14
#define SHIFT_REG_DATA_PIN 15 // GP15

// Total number of shift register outputs
#define SHIFT_REG_OUTPUTS 16

// NOTE rows are held high,
#define MATRIX_ROWS 6
#define MATRIX_COLS 10
#define NUM_MATRIX_KEYS (MATRIX_ROWS * MATRIX_COLS)

#endif

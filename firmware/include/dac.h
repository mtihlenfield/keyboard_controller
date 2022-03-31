#ifndef __DAC_H__
#define __DAC_H__

#define DAC_AMPLIFIER_GAIN 3.2

struct dac;

/* 
 * Function called to set the output voltage of a DAC
 *
 * Returns 0 on success, 1 otherwise
 */
typedef int (*dac_output_setter_t)(struct dac *dac, float volts);

/* 
 * Function called to initialize the DAC instance
 *
 * Returns 0 on success, 1 otherwise
 */
typedef int (*dac_init_t)(struct dac *dac);

struct dac {
    dac_output_setter_t set_output;
    dac_init_t init;
    void *priv;
};

#define dac_set_output(dac, volts) (dac)->set_output((dac), (volts) / DAC_AMPLIFIER_GAIN)
#define dac_init(dac) (dac)->init((dac))

#endif

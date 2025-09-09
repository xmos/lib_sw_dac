// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include <stdint.h>
#include <xcore/clock.h>
#include <xcore/port.h>
#include <xcore/chanend.h>
#include <xcore/parallel.h>

#ifndef __software_dac_sf_h__
#define __software_dac_sf_h__

#include "sigma_delta_modulators.h"

// Keep in sync with sdac_sf.h

#define CHANNELS    2
#define PWM_MAX_LEN 12

typedef struct {
    // These are variables used in the assembly code
    // Many of them point to data further on
    
    int *sigma_delta_state[CHANNELS];
    int32_t *pwm_lookup;
    xclock_t clock_block;
    chanend_t input_chan_end; // For use inside run_dac
    int bank;
    int *sd_coeffs;
    port_t out_ports[CHANNELS];  // Two ports
    int32_t *filter0[CHANNELS];
    int32_t *filter1[CHANNELS];
    int32_t *filter2[CHANNELS];
    int32_t *filter3[CHANNELS];
    int32_t *filter4[CHANNELS];
    int32_t *filter5[CHANNELS];
    
    // Actual data starts here

    int average[CHANNELS];
    
    int32_t pwm_lookup_table[PWM_MAX_LEN];
    
    // Must be multiple of 25 plus 7 overhang for VSTR
    int circular_buffer[100 * 2 + 7];
    
    // This is the input to filter_2x and needs a 40 long buffer
    // The shuffle writes to element [-1..38] inclusive
    // So we create a buffer of 41 and use elements [1..39] inclusive
    int32_t filter_input[CHANNELS][41];

    // This is the input to filter_4x and needs a 16 long input
    // The shuffle writes to element [-2..13] inclusive
    // filter_2x writes to elements [14,15] and destroys [16..21] inclusive
    // So we create a buffer of 24 and use elements [2..17] inclusive
    int32_t filter_stage1_out[CHANNELS][24];
    
    // This is the input to filter_8x and needs a 12 long input
    // The shuffle writes to element [-4..11] inclusive
    // filter_4x writes to elements [12..15] and destroys [16..19] inclusive
    // So we create a buffer of 24 and use elements [4..15] inclusive
    int32_t filter_stage2_out[CHANNELS][24];

    // This is the output of filter_8x
    // It contains 16 samples. It writes to the last 8. 8 are history
    int32_t filter_stage3_out[CHANNELS][16];

    // This is the output of filter_16x
    // It contains 24 samples. It writes to the last 16. 8 are history
    int32_t filter_stage4_out[CHANNELS][24];

    // This is the output of filter_125_3x
    // It contains 41 or 42 samples.
    int32_t filter_stage5_out[CHANNELS][42];
    
    int32_t pre_distort_in_[CHANNELS][33];
    int32_t pre_distort_pwm_comp_history_[CHANNELS][34];
    int32_t pre_distort_flat_comp_history_[CHANNELS][33];
    int32_t *pre_distort_in[CHANNELS];
    int32_t *pre_distort_pwm_comp_history[CHANNELS];
    int32_t *pre_distort_flat_comp_history[CHANNELS];
    int32_t scale1[8];
    int32_t scale2[8];
    int32_t comp_px3_px2_fx2_fx3[32];

    int sigma_delta_state_array[CHANNELS][11];  // TODO: make this 10 (8 + (8-6))
} sw_dac_sf_t;

/**
 * Function that initialises a software DAC. It requires an array of two
 * ports (Left and Right, both 1-bit ports), and a clock block.
 *
 * \param   sd          Software DAC structure, typically allocated on the stack
 *
 * \param   dac_ports   Array of two port identifiers. The ports should not be
 *                      initialised or enabled. Ie, just pass {XS1_PORT_1B, XS1_PORT_1A}
 *                      from C
 *
 * \param   clk         A clock block for the software DAC to use. It will run
 *                      at 50 MHz but will not be started until the DAC is running.
 *
 * \param   max_pwm     number of levels in PWM. Set to an even number to use half
 *                      levels and maximum dynamic range
 *
 * \param   modulator_coefficients  matrix with modular coefficients
 *                  The ``_sf`` version uses a sixth order filter and
 *                  will need a matrix with 8 columsn and 6 rows.
 *
 *                  - Row K calculates state[5-K]
 *
 *                  - Col 0 is the coefficient for the input value
 *
 *                  - Col 1 is the coefficient for the input value
 *
 *                  - Col K is the coefficient for the state[7-K]
 *
 *                  Coefficients may be found in "sigma_delta_modulators.h"
 *
 * \param  scale    What FS should map to in PWM steps, must be < limit
 *
 * \param  limit    Absolute limit for scaled value in PWM steps
 *
 * \param  f_x2     Flat compensation factor, x^2 term
 *
 * \param  f_x3     Flat compensation factor, x^3 term
 *
 * \param  p_x2     PWM (filtered) compensation factor, x^2 term
 *
 * \param  p_x3     PWM (filtered) compensation factor, x^3 term
 */
void sw_dac_sf_init(sw_dac_sf_t *sd,
                    port_t dac_ports[2],
                    xclock_t clk,
                    int max_pwm,
                    int modulator_coefficients[6][8],
                    float scale, float limit,
                    float f_x2, float f_x3,
                    float p_x2, float p_x3);

/**
 * Function that runs a software DAC. Samples are provided over the channel end.
 * The default sample rate is 48,000 Hz. The sample rate may be changed by sending
 * an END token into the channel end followed by the new sample rate. Sending a
 * zero sample rate will terminate the function.
 * This task internally spawns two threads.
 *
 * Sample rates should only be changed after a sequence of zeroes has been sent through
 * the channel end, otherwise a big pop may happen.
 * 
 * \param   sd     Software DAC structure. This must have been initialised with 
 *                 sw_dac_init()
 * 
 * \param   ce     Channel end over which samples are supplied
 *                 Samples must be supplied at exactly 48 kHz according to the
 *                 external clock. First Left then Right.
 */
#ifdef __DOXYGEN__
void sw_dac_sf(sw_dac_sf_t *sd, chanend_t ce);
#else
DECLARE_JOB(sw_dac_sf, (sw_dac_sf_t *, chanend_t)); // Allow calling from PAR_JOB
#endif
/**
 * Assembly kernel that runs the sigma-delta.
 */
void sigma_delta_1_5(sw_dac_sf_t *sd, chanend_t ce); // does not return

#endif

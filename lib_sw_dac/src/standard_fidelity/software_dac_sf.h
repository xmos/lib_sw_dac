// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include <stdint.h>
#include <xcore/clock.h>
#include <xcore/port.h>
#include <xcore/chanend.h>
#include <xcore/hwtimer.h>
#include <xcore/parallel.h>

#ifndef __software_dac_sf_h__
#define __software_dac_sf_h__

#include "sigma_delta_modulators.h"

// Keep in sync with sdac_sf.h

#define CHANNELS    2
#define PWM_MAX_LEN 12

#if CHANNELS != 2
#error "Only stereo build is supported"
#endif

typedef struct {
    // These are variables used in the assembly code
    // Many of them point to data further on

    // Sigma-delta
    int *sigma_delta_state[CHANNELS];
    int32_t *pwm_lookup;
    xclock_t clock_block;
    int *sd_coeffs;
    port_t out_ports[CHANNELS];
    int timeout_period;
    int timeout_word;
    hwtimer_t timeout_resid;
    int timeout_occurred;

    // Upsampling filters
    int32_t *filter0[CHANNELS];
    int32_t *filter1[CHANNELS];
    int32_t *filter2[CHANNELS];
    int32_t *filter3[CHANNELS];
    int32_t *filter4[CHANNELS];
    int bank;

    // DC removal
    int average[CHANNELS];

    // Actual data starts here

    // Buffer to hold data of the pwm_lookup pointer
    int32_t pwm_lookup_table[PWM_MAX_LEN];

    // Buffer to hold data of the filter0 pointer
    // Needs to be long enough to accommodate filter_x2_i1_o2_n80
    // which needs a 40 var long state buffer
    // and will shuffle the state to [-1..38] inclusive
    // so needs to contain 41 elements
    int32_t filter_input[CHANNELS][41];

    // Buffer to hold data of the filter1 pointer
    // Needs to be long enough to accommodate filter_x2_i2_o4_n32
    // which needs a 16 var long state buffer
    // and will shuffle the state to [-2..13] inclusive
    // previous filter will write to the [14..15] and overwrite [16..21] inclusive
    // so needs to contain 24 elements
    int32_t filter_stage1_out[CHANNELS][24];

    // Buffer to hold data of the filter2 pointer
    // Needs to be long enough to accommodate filter_x2_i4_o8_n16
    // which needs a 16 var long state buffer
    // previous filter will write to the [12..15] and overwrite [16..19] inclusive
    // so needs to contain 20 elements, rounded up to a whole number of vectors 24
    int32_t filter_stage2_out[CHANNELS][24];
    
    // Buffer to hold data of the filter3 pointer
    // Needs to be long enough to accommodate filter_x2_i8_o16_n16
    // which needs a 16 var long state buffer
    int32_t filter_stage3_out[CHANNELS][24];

    // Buffer to hold data of the filter4 pointer
    // Needs to be long enough to accommodate filter_x125_64_i16_o32_n16_phased
    // which needs a 24 var long state buffer
    int32_t filter_stage4_out[CHANNELS][32];
    
    int32_t pre_distort_in_[CHANNELS][42];
    int32_t pre_distort_pwm_comp_history_[CHANNELS][43];
    int32_t pre_distort_flat_comp_history_[CHANNELS][42];
    int32_t *pre_distort_in[CHANNELS];
    int32_t *pre_distort_pwm_comp_history[CHANNELS];
    int32_t *pre_distort_flat_comp_history[CHANNELS];
    int32_t scale[16];
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
 * Warning: The output power stage must never be enabled unless the sw_dac_sf is active.
 * Failure to observe this will result in full scale DC being driven to the output.
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


#endif

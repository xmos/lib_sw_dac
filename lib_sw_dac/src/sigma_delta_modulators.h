// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _sigma_delta_modulators_h_
#define _sigma_delta_modulators_h_

/**
 * Coefficients for a fourth order filter for a 1.5 MHz PWM signals with 8 levels
 * -3.5, -2.5, ..., 3.5
 * Note that this needs a change to the modulator code - 0.5 offset.
 */
extern int sd_coeffs_o4_f1_5_n8[6][8];

/**
 * Coefficients for a sixth order filter for a 1.5 MHz PWM signals with 8 levels
 * -3.5, -2.5, ..., 3.5
 */
extern int sd_coeffs_o6_f1_5_n8[6][8];

/**
 * Coefficients for a sixth order filter for a 1.5 MHz PWM signals with 8 levels
 * -3.5, -2.5, ..., 3.5
 */
extern int sd_coeffs_o6_f3_0_n8[6][8];

#endif

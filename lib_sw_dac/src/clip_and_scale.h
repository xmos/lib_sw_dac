// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>

/**
 * output = (input * scale1) * scale2
 * scale1 will cause clipping, scale1*scale2 is the final scaling amount
 * These are #defines.
 *
 * \param y         an output vector for 32 ints
 * \param x         an output vector for 32 ints, Q2.30
 * \param scale1    Clipping scale, should be a number close to 2.0
 * \param scale2    Scale to PWM level, should be a number near pwm_levels*(1<<27)
 * \param n_vec     Number of vectors to apply to, points/8
 */
void clip_and_scale(int32_t y[], int32_t x[],
                    int32_t scale1[], int32_t scale2[],
                    int n_vec);

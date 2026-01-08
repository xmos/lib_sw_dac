// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/**
 * after the call data should be copied as follows:
 * 
 * \param out                an array of 32 integers
 * \param x_history          an array of 33 integers with the pointer passed in to element 1
 * \param pwm_comp_history   an array of 34 integers with the pointer passed in to element 2
 * \param flat_comp_history  an array of 33 integers with the pointer passed in to element 1
 * \param comp_px3_px2_fx2_fx3  an array containint px3 (8x), px2 (8x), fx2 (8x) fx3 (8x) all in Q2.30
 * \param n                  Number of values to run over
 * \param scale              scale-factor1 (replicated 8 times), scale-factor 2 (replicated 8 times)
 */
void pre_distort(int32_t out[], int32_t x_history[], int32_t pwm_comp_history[],
                 int32_t flat_comp_history[], int32_t comp_px3_px2_fx2_fx3[],
                 int n_vectors,
                 int32_t scale[]);

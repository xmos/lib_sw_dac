// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _filter_banks_h_
#define _filter_banks_h_

#include <stdint.h>
#include "sw_dac.h"

// x2 48k -> 96k
extern int32_t filter_linear_81[81];
extern int32_t filter_hashed_81_2[5][2][8]; // x2_i1_o2_n80

// x2 96k -> 192k
extern int32_t filter_linear_25[25];
extern int32_t filter_hashed_25_4[2][4][8]; // x2_i2_o4_n32
extern int32_t filter_hashed_25_4__[2][2][8]; // x2_i1_o2_n32

// x2 192k -> 386k
extern int32_t filter_8x_linear[16];
extern int32_t filter_8x_coefficients[2][8]; // to x2_i8_o16_n16, x2_i4_o8_n16, x2_i2_o4_n16
extern int32_t filter_8x_coefficients__[2][8]; // x2_i1_o2_n16

// x2 386k -> 768k
extern int32_t filter_16x_linear[16];
extern int32_t filter_16x_coefficients[2][8]; // x2_i8_o16_n16, x2_i4_o8_n16, x2_i2_o4_n16

// poly x125/64 768 -> 1500
extern int32_t filter_banks_125_64_banks[16][32][8]; // x125_64_i4_o8_n16_phase_<>

#define FILTER_HEADROOM 1
#define ATT(x) ((x))
//#define A(x) ((int)(((x) * 0.5125)+0.5))
//#define A(x) ((x+2)>>2)

#define A(x) (x) // ((int)(((x) * SIGMA_DELTA_MULTIPLIER * 0.3568)+0.5))

// x2 1 -> 2 samples
void filter_x2_i1_o2_n80(int32_t *output, int32_t *input, int32_t *filter);
void filter_x2_i1_o2_n32(int32_t *output, int32_t *input, int32_t *filter);
void filter_x2_i1_o2_n16(int32_t *output, int32_t *input, int32_t *filter);
// x2 2 -> 4 samples
void filter_x2_i2_o4_n32(int32_t *output, int32_t *input, int32_t *filter);
void filter_x2_i2_o4_n16(int32_t *output, int32_t *input, int32_t *filter);
// x2 4 -> 8 samples
void filter_x2_i4_o8_n16(int32_t *output, int32_t *input, int32_t *filter);
// x2 8 -> 16 samples
void filter_x2_i8_o16_n16(int32_t *output, int32_t *input, int32_t *filter);
// poly x125/64 ~ x1.953 
void filter_x125_64_i4_o8_n16_phase_0123489cde(int32_t *output, int32_t *input, int32_t *filter);
void filter_x125_64_i4_o8_n16_phase_67b(int32_t *output, int32_t *input, int32_t *filter);
void filter_x125_64_i4_o8_n16_phase_5(int32_t *output, int32_t *input, int32_t *filter);
void filter_x125_64_i4_o8_n16_phase_a(int32_t *output, int32_t *input, int32_t *filter);
void filter_x125_64_i4_o8_n16_phase_f(int32_t *output, int32_t *input, int32_t *filter);

void filter_shuffle_n24_n8(int32_t *samples);
void filter_shuffle_n24_n16(int32_t *samples);
void filter_shuffle_n24_n20(int32_t *samples);
#endif

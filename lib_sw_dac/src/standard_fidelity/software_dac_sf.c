// Copyright 2024-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <xcore/port.h>
#include <xcore/clock.h>
#include <xcore/channel.h>
#include <xcore/parallel.h>
#include "software_dac_sf.h"
#include <xcore/port.h>
#include "sdac_sf.h"
#include "filter_banks.h"
#include "sw_dac_conf_default.h"
#include "pre_distort.h"


static void init_ports(port_t dac_ports[2], xclock_t clk) {
    for(int i = 0; i < 2; i++) {
        port_start_buffered(dac_ports[i], 32);
        port_set_clock(dac_ports[i], clk);
        port_clear_buffer(dac_ports[i]);
    }
}

static uint64_t mkmsk_long(int val) {
    if (val >= 64) {
        return ~0ULL;
    }
    return (1ULL << val) - 1;
}


#include <stdio.h>

void sw_dac_sf_init(sw_dac_sf_t *sd,
                    port_t dac_ports[2], xclock_t clk,
                    int pwm_levels, int sd_coeffs[6][8],
                    float scale, float limit,
                    float f_x2, float f_x3,
                    float p_x2, float p_x3) {
    memset(sd, 0, sizeof(*sd));
    init_ports(dac_ports, clk);
    sd->sd_coeffs = &sd_coeffs[0][0];
    const int negate = SW_DAC_NEGATE ? -1 : 1;
    
    for(int i = 0; i < 8; i++) {   // TODO: the compiler probably hoists these constants.
        sd->scale1[i] = 0x40000000 * scale / limit * 2;
        sd->scale2[i] = 0x40000000 * limit / 4 / 2 * negate;      // TODO: /4   depends on Q4.28
        sd->comp_px3_px2_fx2_fx3[i+8*0] = 0x40000000 * p_x3;
        sd->comp_px3_px2_fx2_fx3[i+8*1] = 0x40000000 * p_x2;
        sd->comp_px3_px2_fx2_fx3[i+8*2] = 0x40000000 * f_x2;
        sd->comp_px3_px2_fx2_fx3[i+8*3] = 0x40000000 * f_x3;
    }
//    printf("%d %d %d %d\n", sd->comp_px3_px2_fx2_fx3[0], sd->comp_px3_px2_fx2_fx3[8], sd->comp_px3_px2_fx2_fx3[16], sd->comp_px3_px2_fx2_fx3[24]);
    for(int i = 0; i < CHANNELS; i++) {
        sd->filter0[i] = &sd->filter_input[i][1];
        sd->filter1[i] = &sd->filter_stage1_out[i][2];
        sd->filter2[i] = &sd->filter_stage2_out[i][4];
        sd->filter3[i] = &sd->filter_stage3_out[i][0];
        sd->filter4[i] = &sd->filter_stage4_out[i][0];
        sd->filter5[i] = &sd->filter_stage5_out[i][0];
        sd->sigma_delta_state[i] = &sd->sigma_delta_state_array[i][0];
        sd->pre_distort_in[i] = &sd->pre_distort_in_[i][1];
        sd->pre_distort_pwm_comp_history[i] = &sd->pre_distort_pwm_comp_history_[i][2];
        sd->pre_distort_flat_comp_history[i] = &sd->pre_distort_flat_comp_history_[i][1];
    }
    assert(pwm_levels <= PWM_MAX_LEN);
    sd->clock_block = clk;
    memcpy(&sd->out_ports[0], dac_ports, 2*sizeof(int));

    // PWMs are centered around the high bit

    // Even number
    // 0000 0001 0000 0000 -3.5 -4
    // 0000 0011 1000 0000 -2.5 -3
    // 0000 0111 1100 0000 -1.5 -2
    // 0000 1111 1110 0000 -0.5 -1
    // 0001 1111 1111 0000 0.5 0
    // 0011 1111 1111 1000 1.5 1
    // 0111 1111 1111 1100 2.5 2
    // 1111 1111 1111 1110 3.5 3

    // Odd number
    // 0000 0001 1000 0000 -3
    // 0000 0011 1100 0000 -2
    // 0000 0111 1110 0000 -1 
    // 0000 1111 1111 0000 0
    // 0001 1111 1111 1000 1
    // 0011 1111 1111 1100 2
    // 0111 1111 1111 1110 3

    int pwm_max = pwm_levels >> 1;
    if ((pwm_levels & 1) == 0) {
        sd->pwm_lookup = &sd->pwm_lookup_table[pwm_max+1];
        for(int i = -pwm_max; i <= pwm_max-1; i++) {           // Assymetric, one more at -N then at +N-1
            uint64_t first_64;
            uint64_t mask          = mkmsk_long(i+pwm_max);
            uint64_t mask_reversed = mask << (2*pwm_max-(i+pwm_max));
            first_64 = mask_reversed | (1 << (2*pwm_max)) | (mask << (1 + 2*pwm_max));
            sd->pwm_lookup[i] = first_64;
//            printf("%08x\n", (int) sd->pwm_lookup[i]);
        }
    } else {
        sd->pwm_lookup = &sd->pwm_lookup_table[pwm_max];
        for(int i = -pwm_max; i <= pwm_max; i++) {
            uint64_t first_64;
            uint64_t mask          = mkmsk_long(i+pwm_max);
            uint64_t mask_reversed = mask << (2*pwm_max-(i+pwm_max));
            first_64 = mask_reversed | (3 << (2*pwm_max)) | (mask << (2 + 2*pwm_max));
            sd->pwm_lookup[i] = first_64;
//        printf("%08x\n", (int) sd->pwm_lookup[i]);
        }
    }
}

DECLARE_JOB(filter_task,      (sw_dac_sf_t *, chanend_t, chanend_t));
DECLARE_JOB(sigma_delta_task, (sw_dac_sf_t *, chanend_t));

static inline int filter_x125_64_i16_o32_n16_phased(sw_dac_sf_t *sd, int32_t *output, int ch, int32_t samples[16]) {
    switch(sd->bank) {
    case 0:
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[0],  samples,    &filter_banks_125_64_banks[0][0][0]); // 0
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[8],  samples+4,  &filter_banks_125_64_banks[1][0][0]); // 1
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[16], samples+8,  &filter_banks_125_64_banks[2][0][0]); // 2
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[24], samples+12, &filter_banks_125_64_banks[3][0][0]); // 3
        filter_shuffle_n24_n8(samples); // shuffle down 16 place
        return 32;
    case 1:
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[0],  samples, &filter_banks_125_64_banks[4][0][0]); // 4
        filter_x125_64_i4_o8_n16_phase_5         (&output[8],  samples+4, &filter_banks_125_64_banks[5][0][0]); // 5
        filter_x125_64_i4_o8_n16_phase_67b       (&output[16], samples+8, &filter_banks_125_64_banks[6][0][0]); // 6
        filter_x125_64_i4_o8_n16_phase_67b       (&output[24], samples+12, &filter_banks_125_64_banks[7][0][0]); // 7
        filter_shuffle_n24_n8(samples); // shuffle down 16 place
        return 31;
    case 2:
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[0],  samples, &filter_banks_125_64_banks[8][0][0]); // 8
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[8],  samples+4, &filter_banks_125_64_banks[9][0][0]); // 9
        filter_x125_64_i4_o8_n16_phase_a         (&output[16], samples+8, &filter_banks_125_64_banks[10][0][0]); // 10
        filter_x125_64_i4_o8_n16_phase_67b       (&output[24], samples+12, &filter_banks_125_64_banks[11][0][0]); // 11
        filter_shuffle_n24_n8(samples); // shuffle down 16 place
        return 31;
    default:
    case 3:
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[0],  samples, &filter_banks_125_64_banks[12][0][0]); // 12
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[8],  samples+4, &filter_banks_125_64_banks[13][0][0]); // 13
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[16], samples+8, &filter_banks_125_64_banks[14][0][0]); // 14
        filter_x125_64_i4_o8_n16_phase_f         (&output[24], samples+12, &filter_banks_125_64_banks[15][0][0]); // 15
        filter_shuffle_n24_n8(samples); // shuffle down 16 place
        return 31;
    }

    (void)ch; // TODO remove this argument from function if not needed
}

static inline int filter_x125_64_i8_o16_n16_phased(sw_dac_sf_t *sd, int32_t *output, int ch, int32_t samples[16]) {
    int32_t *filter_bank  = &filter_banks_125_64_banks[2*sd->bank+0][0][0];
    int32_t *filter_bank1 = &filter_banks_125_64_banks[2*sd->bank+1][0][0];
    switch(sd->bank) {
    default:
    case 0:
    case 1:
    case 4:
    case 6:
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[0], samples,   filter_bank); // 0, 2, 8, 12
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[8], samples+4, filter_bank1); // 1, 3, 9, 13
        filter_shuffle_n24_n16(samples); // shuffle down 8 place
        return 16;
    case 2:
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[0], samples,   filter_bank); // 4
        filter_x125_64_i4_o8_n16_phase_5         (&output[8], samples+4, filter_bank1); // 5
        filter_shuffle_n24_n16(samples); // shuffle down 8 place
        return 16;
    case 3:
        filter_x125_64_i4_o8_n16_phase_67b       (&output[0], samples,   filter_bank); // 6
        filter_x125_64_i4_o8_n16_phase_67b       (&output[8], samples+4, filter_bank1); // 7
        filter_shuffle_n24_n16(samples); // shuffle down 8 place
        return 15;
    case 5:
        filter_x125_64_i4_o8_n16_phase_a         (&output[0], samples,   filter_bank); // 10
        filter_x125_64_i4_o8_n16_phase_67b       (&output[8], samples+4, filter_bank1); // 11
        filter_shuffle_n24_n16(samples); // shuffle down 8 place
        return 15;
    case 7:
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[0], samples,   filter_bank); // 14
        filter_x125_64_i4_o8_n16_phase_f         (&output[8], samples+4, filter_bank1); // 15
        filter_shuffle_n24_n16(samples); // shuffle down 8 place
        return 15;
    }

    (void)ch; // TODO remove this argument from function if not needed
}

static inline int filter_x125_64_i4_o8_n16_phased(sw_dac_sf_t *sd, int32_t *output, int ch, int32_t samples[16]) {
    int32_t *filter_bank = &filter_banks_125_64_banks[sd->bank][0][0];
    switch(sd->bank) {
    case 5:
        filter_x125_64_i4_o8_n16_phase_5         (&output[0], samples, filter_bank); // 5
        filter_shuffle_n24_n20(samples); // shuffle down 4 place
        return 8;
    case 6:
        filter_x125_64_i4_o8_n16_phase_67b       (&output[0], samples, filter_bank); // 5
        filter_shuffle_n24_n20(samples); // shuffle down 4 place
        return 8;
    case 11:
    case 7:
        filter_x125_64_i4_o8_n16_phase_67b       (&output[0], samples, filter_bank); // 5
        filter_shuffle_n24_n20(samples); // shuffle down 4 place
        return 7;
    case 10:
        filter_x125_64_i4_o8_n16_phase_a         (&output[0], samples, filter_bank); // 10
        filter_shuffle_n24_n20(samples); // shuffle down 4 place
        return 8;
    case 15:
        filter_x125_64_i4_o8_n16_phase_f         (&output[0], samples, filter_bank); // 15
        filter_shuffle_n24_n20(samples); // shuffle down 4 place
        return 7;
    default:
        filter_x125_64_i4_o8_n16_phase_0123489cde(&output[0], samples, filter_bank);
        filter_shuffle_n24_n20(samples); // shuffle down 4 place
        return 8;
    }

    (void)ch; // TODO remove this argument from function if not needed
}

#if SW_DAC_DC_REMOVAL_TIME_CONSTANT

// Set this to 14 to get a cut-off of just under 1 Hz.

#define DC_REMOVAL_ALPHA_48  (1<<(32-SW_DAC_DC_REMOVAL_TIME_CONSTANT))
#define DC_REMOVAL_ALPHA_96  (DC_REMOVAL_ALPHA_48 >> 1)
#define DC_REMOVAL_ALPHA_192 (DC_REMOVAL_ALPHA_48 >> 2)

static int dc_removal(sw_dac_sf_t *sd, int ch, int sample, int alpha) {
    int average = sd->average[ch];
    sd->average[ch] = (average * (int64_t) (uint32_t) ~alpha + sample * (int64_t) alpha) >> 32;
    return sample - average;
}

#else

#define DC_REMOVAL_ALPHA_48  0
#define DC_REMOVAL_ALPHA_96  0
#define DC_REMOVAL_ALPHA_192 0

static int dc_removal(sw_dac_sf_t *sd, int ch, int sample, int alpha) {
    return sample;
}

#endif

// times 125 divide by 4: 48 -> 1500
int filter_x125_4(sw_dac_sf_t *sd, int32_t *output, int ch, int32_t sample) {
    sd->filter0[ch][39] = dc_removal(sd, ch, sample, DC_REMOVAL_ALPHA_48);
    filter_x2_i1_o2_n80(&sd->filter1[ch][14], sd->filter0[ch], &filter_hashed_81_2[0][0][0]);
    filter_x2_i2_o4_n32(&sd->filter2[ch][12], sd->filter1[ch], &filter_hashed_25_4[0][0][0]);      // 25_4 is incorrectly defined and duplicated
    filter_x2_i4_o8_n16(&sd->filter3[ch][8], sd->filter2[ch], &filter_8x_coefficients[0][0]);
    filter_x2_i8_o16_n16(&sd->filter4[ch][8], sd->filter3[ch], &filter_16x_coefficients[0][0]);
    return filter_x125_64_i16_o32_n16_phased(sd, output, ch, &sd->filter4[ch][0]);
}

// times 125 divide by 8: 96 -> 1500
int filter_x125_8(sw_dac_sf_t *sd, int32_t *output, int ch, int32_t sample) {
    sd->filter0[ch][15] = dc_removal(sd, ch, sample, DC_REMOVAL_ALPHA_96);
    // Input samples filter0[0..15], Two FIRs (even odd) on [0..15] produce two samples
    // Output samples go into filter1[7..8], so that there are 9 samples in filter1
    filter_x2_i1_o2_n32(&sd->filter1[ch][7], sd->filter0[ch], &filter_hashed_25_4__[0][0][0]);
    // Input samples filter1[0..8], Two FIRs (even odd) on [0..7] produce two samples, two FIRs (even odd) on [1..8] produce two more samples
    // Output samples go into filter2[7..10], so that there are 11 samples in filter2
    filter_x2_i2_o4_n16(&sd->filter2[ch][7], sd->filter1[ch], &filter_8x_coefficients[0][0]);
    // Input samples filter2[0..10], Two FIRs (even odd) on [0..7], [1..8], [2..9], [3..10] produce four samples
    // Output samples go into filter3[12..15], so that there are 16 samples in filter3
    filter_x2_i4_o8_n16(&sd->filter3[ch][12], sd->filter2[ch], &filter_16x_coefficients[0][0]);
    return filter_x125_64_i8_o16_n16_phased(sd, output, ch, &sd->filter3[ch][0]);
}

// times 125 divide by 16: 192 -> 1500
int filter_x125_16(sw_dac_sf_t *sd, int32_t *output, int ch, int32_t sample) {
    sd->filter0[ch][7] = dc_removal(sd, ch, sample, DC_REMOVAL_ALPHA_192);;
    // Input samples filter0[0..7], Two FIRs (even odd) on [0..7] produce two samples
    // Output samples go into filter1[7..8], so that there are 9 samples in filter1
    filter_x2_i1_o2_n16(&sd->filter1[ch][7], sd->filter0[ch], &filter_8x_coefficients__[0][0]);
    // Input samples filter1[0..8], Two FIRs (even odd) on [0..7] produce two samples, two FIRs (even odd) on [1..8] produce two more samples
    // Output samples go into filter2[12..15], so that there are 16 samples in filter2
    filter_x2_i2_o4_n16(&sd->filter2[ch][12], sd->filter1[ch], &filter_16x_coefficients[0][0]);
    int n = filter_x125_64_i4_o8_n16_phased(sd, output, ch, &sd->filter2[ch][0]);
    return n;
}

void filter_task(sw_dac_sf_t *sd, chanend_t c_in, chanend_t c_out) {
    int sample_rate = 48000;
    int32_t data[4][SDAC_BUF_TOTAL];
    memset(data, 0, sizeof(data));
    data[2][SDAC_BUF_N] = 32;
    chanend_out_word(c_out, (int) &data[2][0]);
    data[3][SDAC_BUF_N] = 32;
    chanend_out_word(c_out, (int) &data[3][0]);
    int i = 0;
    while(1) {
            if (chanend_test_control_token_next_byte(c_in)) {
                chanend_check_control_token(c_in, XS1_CT_END);
                sample_rate = chanend_in_word(c_in);
            }
            int32_t dataL = chanend_in_word(c_in);
            dataL >>= FILTER_HEADROOM;
            int32_t dataR = chanend_in_word(c_in);
            dataR >>= FILTER_HEADROOM;

            int n, mask;
            switch(sample_rate) {
            case 48000:
                n = filter_x125_4(sd, sd->pre_distort_in[0], 0, dataL);
                (void) filter_x125_4(sd, sd->pre_distort_in[1], 1, dataR);
                mask = 3;
                break;
            case 96000:
                n = filter_x125_8(sd, sd->pre_distort_in[0], 0, dataL);
                (void) filter_x125_8(sd, sd->pre_distort_in[1], 1, dataR);
                mask = 7;
                break;
            case 192000:
                n = filter_x125_16(sd, sd->pre_distort_in[0], 0, dataL);
                (void) filter_x125_16(sd, sd->pre_distort_in[1], 1, dataR);
                mask = 15;
                break;
#if defined(TEST_PRINT)
            case 1500000:
                sd->pre_distort_in[0][0] = dataL;
                sd->pre_distort_in[1][0] = dataR;
                n = 1;
                mask = 1;
                break;
#endif
            }
            int oindex = SDAC_BUF_L;
            int n_vec = (n + 7) >> 3;
            // TODO: make pre_distort and clip_and_scale N long, where N = 8, 16, 32.
            for(int c = 0; c < 2; c++) {
                pre_distort(&data[i][oindex],
                            sd->pre_distort_in[c],
                            sd->pre_distort_pwm_comp_history[c],
                            sd->pre_distort_flat_comp_history[c],
                            sd->comp_px3_px2_fx2_fx3,
                            n_vec,
                            sd->scale1, sd->scale2);
                // roll history
                sd->pre_distort_in               [c][-1] = sd->pre_distort_in               [c][n-1];
                sd->pre_distort_flat_comp_history[c][-1] = sd->pre_distort_flat_comp_history[c][n-1];
                sd->pre_distort_pwm_comp_history [c][-1] = sd->pre_distort_pwm_comp_history [c][n-1];
                sd->pre_distort_pwm_comp_history [c][-2] = sd->pre_distort_pwm_comp_history [c][n-2];

                oindex = SDAC_BUF_R;
            }
            sd->bank = (sd->bank + 1) & mask;
                    
#ifdef TEST_PRINT
            #if 1
            int debug_level = n; // Set to n for highest
            for(int k = 0; k < debug_level; k++) {
                if (k < 1)  printf("%11ld , ", sd->filter0[0][k]); else printf("            , ");
                if (k < 2)  printf("%11ld , ", sd->filter1[0][k]); else printf("            , ");
                if (k < 4)  printf("%11ld , ", sd->filter2[0][k]);  else printf("            , ");
                if (k < 8)  printf("%11ld , ", sd->filter3[0][k]);  else printf("            , ");
                if (k < 16) printf("%11ld , ", sd->filter4[0][k]);  else printf("            , ");
                printf("%11ld\n", data[i][SDAC_BUF_L + k]);
            }
            #endif
#endif
            data[i][SDAC_BUF_N] = n;
#ifndef TEST_PRINT
            chanend_out_word(c_out, (int) &data[i][0]);
#endif
            i = (i+1) & 3;
    }
    chanend_out_control_token(c_out, 1);
}

void sigma_delta_task(sw_dac_sf_t *sd, chanend_t c_in) {
    sigma_delta_1_5(sd, c_in);
}

void sw_dac_sf(sw_dac_sf_t *sd, chanend_t c_in) {
    channel_t c = chan_alloc();

    PAR_JOBS(
        PJOB(filter_task,       (sd, c_in, c.end_a)),
        PJOB(sigma_delta_task,  (sd, c.end_b))
        );
}

// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/port.h>
#include <xcore/channel.h>
#include <xcore/parallel.h>
#include <xcore/thread.h>
#include <xcore/assert.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include "sw_dac.h"


volatile int clk_ready = 0;

// Producer runs at full speed in this test so is throttled by downstream
DECLARE_JOB(test_app, (chanend_t, int, int, int));
void test_app(chanend_t c_sd, int sample_rate, int burn, int n_loops) {
    if(burn){
        local_thread_mode_set_bits(thread_mode_fast); // Always issue
    }

    while(!clk_ready);

    chanend_out_control_token(c_sd, 1);
    chanend_out_word(c_sd, sample_rate);

    for(int i = 0; i < n_loops ; i++) {
        chanend_out_word(c_sd, i);              // Left
        chanend_out_word(c_sd, i + 1000000);    // Right
    }
    printf("Completed test app\n");

    _Exit(0);
}


DECLARE_JOB(clk_gen,(void));
void clk_gen(void){
    // 24 MHz pattern at 100MHz out - low noise below 1.5MHz
    // LCM of 25 and 32 is 800. So that's 25 32b words we need.
    // 25b pattern repeats wrapped for to 800 bits

    int pattern[25] = {1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0};
    uint32_t clk24_pattern[25] = {0};
    // Check we have correct num transitions
    int pattern_idx = 0;
    int old_pattern = 0;
    int num_pos_edges = 0;

    for(int word = 0; word < 25; word++){
        for(int bit = 0; bit < 32; bit++){
            int next_bit = pattern[pattern_idx];

            // Count rising edges
            if(next_bit == 1 && old_pattern == 0){
                num_pos_edges++;
            }
            old_pattern = next_bit;

            clk24_pattern[word] |= (next_bit << bit);
            // printf("word: %d bitpos: %d pattern_idx: %d, num_pos: %d\n", word, bit, next_bit, num_pos_edges);
            // printf("%d", next_bit);
            if(++pattern_idx == 25) pattern_idx = 0;
        }
    }

    // Check we have 24MHz exactly
    xassert(100.0 / (25.0 * 32.0) * (float)num_pos_edges == 24.0);


    port_t clk_out = XS1_PORT_1C;
    xclock_t clk = XS1_CLKBLK_2;
    clock_enable(clk);
    clock_set_source_clk_ref(clk);
    port_enable (clk_out);
    port_set_clock(clk_out, clk);
    port_start_buffered(clk_out, 32);
    port_out(clk_out, 0);

    local_thread_mode_set_bits(thread_mode_fast); // Always issue for burn
    clock_start(clk);
    clk_ready = 1;
    int idx = 0;
    while(1){
        port_out(clk_out, clk24_pattern[idx]);
        if(++idx == 25) idx = 0;
    }
}

DECLARE_JOB(burn, (void));
void burn(void){
    while(1);
}


int main(int argc, char *argv[]) {
    int sample_rate = atoi(argv[1]);
    int burn = atoi(argv[2]);
    int n_loops = atoi(argv[3]);

    channel_t c_sd = chan_alloc();
    xclock_t clk = XS1_CLKBLK_1;
    port_t ports[2] = {XS1_PORT_1A, XS1_PORT_1B};   // L and R outputs

    // Setup clock block to run from MCLK in which is set to 24MHz by the App PLL
    port_t clk_in = XS1_PORT_1C;
    port_enable(clk_in);
    clock_enable(clk);
    clock_set_source_port(clk, clk_in);
    
    sw_dac_sf_t sd;
    sw_dac_sf_init(&sd, ports, clk, 8, sd_coeffs_o6_f1_5_n8,
                    2.8544, 2.8684735298,      // scale, limit
                    1.0/120000, -1.0/250000,   // flat_comp_x2, x3
                    3.0/157, 0.63/157);        // pwm comp x2, x3

    printf("Started test app sr: %d, burn: %d, loops: %d\n", sample_rate, burn, n_loops);

    if(burn){
    PAR_JOBS(
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(clk_gen,           ()),
        PJOB(test_app,          (c_sd.end_a, sample_rate, burn, n_loops)),
        PJOB(sw_dac_sf,         (&sd, c_sd.end_b))
        );
    } else {
    PAR_JOBS(
        PJOB(clk_gen,           ()),
        PJOB(test_app,          (c_sd.end_a, sample_rate, burn, n_loops)),
        PJOB(sw_dac_sf,         (&sd, c_sd.end_b))
        );

    }
}
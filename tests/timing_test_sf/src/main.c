// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/port.h>
#include <xcore/channel.h>
#include <xcore/parallel.h>
#include <xcore/thread.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include "sw_dac.h"


DECLARE_JOB(test_app, (chanend_t, int, int, int));
void test_app(chanend_t c_sd, int sample_rate, int burn, int n_loops) {
    if(burn){
        local_thread_mode_set_bits(thread_mode_fast); // Always issue
    }

    chanend_out_control_token(c_sd, 1);
    chanend_out_word(c_sd, sample_rate);

    for(int i = 0; i < n_loops ; i++) {
        chanend_out_word(c_sd, i);              // Left
        chanend_out_word(c_sd, i + 1000000);    // Right
    }
    printf("Completed test app\n");

    _Exit(0);
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
    port_t clk_out = XS1_PORT_1D;                   // Dummy, unconnected

    // Setup clock block to run from MCLK in which is set to 24MHz by the App PLL
    clock_enable(clk);
    clock_set_source_clk_ref(clk);
    clock_set_divide(clk, 4 / 2); // 25MHz
    
    software_dac_sf_t sd;
    software_dac_sf_init(&sd, ports, clk, clk_out, 8, sd_coeffs_o6_f1_5_n8,
                         2.8544, 2.8684735298,      // scale, limit
                         1.0/120000, -1.0/250000,   // flat_comp_x2, x3
                         3.0/157, 0.63/157,         // pwm comp x2, x3
                         0);                        // negate

    printf("Started test app sr: %d, loops: %d, burn: %d\n", sample_rate, burn, n_loops);

    if(burn){
    PAR_JOBS(
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(test_app,          (c_sd.end_a, sample_rate, burn, n_loops)),
        PJOB(software_dac_sf,   (&sd, c_sd.end_b))
        );
    } else {
    PAR_JOBS(
        PJOB(test_app,          (c_sd.end_a, sample_rate, burn, n_loops)),
        PJOB(software_dac_sf,   (&sd, c_sd.end_b))
        );

    }
}
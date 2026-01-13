// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/port.h>
#include <xcore/channel.h>
#include <xcore/parallel.h>
#include <xcore/assert.h>
#include <xcore/thread.h>
#include <xcore/hwtimer.h>
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include "sw_dac.h"


volatile int n_restarts = 0;


DECLARE_JOB(test_app, (chanend_t));
void test_app(chanend_t c_sd) {
    int app_exits = 0;
    int srs[] = {44100, 48000, 88200, 96000, 176400, 192000};
    int sr_idx = 0;

    // Do this enough times to ensure we don't have any resource leaks (max chanends=32)
    for(int n_loops = 0; n_loops < 32; n_loops++){
        int sr = srs[sr_idx];
        if(++sr_idx == (sizeof(srs) / sizeof(srs[0]))) sr_idx = 0;
        chanend_out_control_token(c_sd, XS1_CT_END);
        chanend_out_word(c_sd, sr);

        // Do some streaming - a few tens of samples is plenty
        for(int i = 0; i < 20; i++) {
            chanend_out_word(c_sd, i);              // Left
            chanend_out_word(c_sd, i + 1000000);    // Right
        }
        chanend_out_control_token(c_sd, XS1_CT_END);
        chanend_out_word(c_sd, 0);
        printf("Sent quit: %d\n", app_exits);

        app_exits++;
    }
    hwtimer_t tmr = hwtimer_alloc();
    hwtimer_delay(tmr, 100000); // Allow SD to exit
    hwtimer_free(tmr);


    printf("Completed test app. App_exits: %d sd_restarts: %d\n", app_exits, n_restarts);

    xassert(app_exits == n_restarts);

    _Exit(0);
}


DECLARE_JOB(sw_dac_wrapper, (chanend_t));
void sw_dac_wrapper(chanend_t c_sd){
    xclock_t clk = XS1_CLKBLK_1;
    port_t ports[2] = {XS1_PORT_1A, XS1_PORT_1B};   // L and R outputs

    // Setup clock block to run from MCLK in which is set to 24MHz by the App PLL
    clock_enable(clk);
    clock_set_source_clk_ref(clk);
    clock_set_divide(clk, 4 / 2); // 25MHz
    
    sw_dac_sf_t sd;

    while(1){
        sw_dac_sf_init(&sd, ports, clk, 8, sd_coeffs_o6_f1_5_n8,
                        2.8544, 2.8684735298,      // scale, limit
                        1.0/120000, -1.0/250000,   // flat_comp_x2, x3
                        3.0/157, 0.63/157);        // pwm comp x2, x3


        sw_dac_sf(&sd, c_sd);
        printf("SD exit: %d\n", n_restarts);
        n_restarts++;
    }
}


int main(void) {
    channel_t c_sd = chan_alloc();

    printf("Started test app\n");

    PAR_JOBS(
        PJOB(test_app,          (c_sd.end_a)),
        PJOB(sw_dac_wrapper,    (c_sd.end_b))
        );
}

// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/port.h>
#include <xcore/channel.h>
#include <xcore/parallel.h>
#include <xcore/thread.h>
#include <xcore/select.h>
#include <xcore/hwtimer.h>
#include <xs1.h>
#include <stdio.h>
#include <print.h>
#include <stdlib.h>
#include "sw_dac.h"
#include "sdac_sf.h"
#include "sigma_delta_modulators.h"

#include "../../common/sin1500.h"
#include <xscope.h>

DECLARE_JOB(sigma_delta_task_sf,    (sw_dac_sf_t *, chanend_t));


const int n_sd_loops = 2;

DECLARE_JOB(test_app, (sw_dac_sf_t *, chanend_t, int, int));
void test_app(sw_dac_sf_t *sd, chanend_t c_sd_in, int burn, int n_loops) {
    xscope_mode_lossless();
    
    if(burn){
        local_thread_mode_set_bits(thread_mode_fast); // Always issue if enabled
    }

    const int num_buffs_in_sd = 4;

    int32_t data[num_buffs_in_sd][SDAC_BUF_TOTAL] = {{0}};
    int sample_idx = 0;

    hwtimer_t tmr = hwtimer_alloc();

    for(int loop_count = 0; loop_count < n_loops; loop_count++){
        // printf("loop: %d\n", loop_count);
        // xscope_int(0, loop_count);

        int idx = loop_count % num_buffs_in_sd;
        
        // Send to SD modulator/PWM
        // First setup array to transfer to SD
        data[idx][SDAC_BUF_N] = n_sd_loops;
        for(int i = 0; i < n_sd_loops; i++){
            int32_t sample = sin1500[sample_idx % 1500] >> 2; // More amplitude than this and it clips/ overflows
            data[idx][SDAC_BUF_L + i] = sample;
            data[idx][SDAC_BUF_R + i] = -sample;
            sample_idx++;
            if(sample_idx == 50){
                hwtimer_delay(tmr, 3000);
            }
        }

        chanend_out_word(c_sd_in, (int) &data[idx][0]);

    }

    printf("Completed test app, timed out: %d\n", sd->timeout_occurred);
    _Exit(0);
}

DECLARE_JOB(burn, (void));
void burn(void){
    while(1);
}




int main(int argc, char *argv[]) {
    if(argc != 3){
        printf("Error - need to pass burn and loops as args\n");
        _Exit(-1);
    }
    int burn = atoi(argv[1]);
    int n_loops = atoi(argv[2]);

    printf("Started test app, loops: %d, burn: %d\n", n_loops, burn);


    channel_t c_sd_ip = chan_alloc();
    port_t p_left = XS1_PORT_1A;
    port_t p_right = XS1_PORT_1B;

    xclock_t clk = XS1_CLKBLK_1;
    port_t ports[2] = {p_left, p_right};   // L and R outputs

    // Setup clock block to run from MCLK in which is set to 24MHz by the App PLL
    clock_enable(clk);
    clock_set_source_clk_ref(clk);
    clock_set_divide(clk, 4 / 2); // 25MHz
    
    sw_dac_sf_t sd;
    sw_dac_sf_init(&sd, ports, clk, 8, sd_coeffs_o6_f1_5_n8,
                    2.8544, 2.8684735298,      // scale, limit
                    1.0/120000, -1.0/250000,   // flat_comp_x2, x3
                    3.0/157, 0.63/157);        // pwm comp x2, x3

    if(burn){
    PAR_JOBS(
        PJOB(burn,                  ()),
        PJOB(burn,                  ()),
        PJOB(burn,                  ()),
        PJOB(burn,                  ()),
        PJOB(burn,                  ()),
        PJOB(test_app,              (&sd, c_sd_ip.end_a, burn, n_loops)),
        PJOB(sigma_delta_task_sf,   (&sd, c_sd_ip.end_b))
        );
    } else {
    PAR_JOBS(
        PJOB(test_app,              (&sd, c_sd_ip.end_a, burn, n_loops)),
        PJOB(sigma_delta_task_sf,   (&sd, c_sd_ip.end_b))
        );

    }
}

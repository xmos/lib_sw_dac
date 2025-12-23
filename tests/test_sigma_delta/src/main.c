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
#include <stdlib.h>
#include "sw_dac.h"
#include "sdac_sf.h"
#include "../../common/sin1500.h"
#include <xscope.h>
#include <print.h>

extern void sigma_delta_task_sf(sw_dac_sf_t * sd, chanend_t c_sd);

int running = 1;


DECLARE_JOB(test_consumer, (chanend_t, chanend_t, int));
void test_consumer(chanend_t port_l, chanend_t port_r, int burn) {
    xscope_mode_lossless();
        
    if(burn){
        local_thread_mode_set_bits(thread_mode_fast); // Always issue
    }

    hwtimer_t tmr = hwtimer_alloc();

    uint32_t time_trig = hwtimer_get_trigger_time(tmr);
    while(running){
        time_trig += (XS1_TIMER_HZ / 1500000); // Throttle consumption of outs because we are working over a chanend
        hwtimer_wait_until(tmr, time_trig);
        unsigned pwm_l = chanend_in_word(port_l);
        unsigned pwm_r = chanend_in_word(port_r);
        // printf("port: 0x%x 0x%x\n", pwm_l, pwm_r);

        xscope_int(0, pwm_l);
        xscope_int(1, pwm_r);
    }
}


DECLARE_JOB(test_producer, (sw_dac_sf_t*, chanend_t, int, int, int));
void test_producer(sw_dac_sf_t *sd, chanend_t c_sd_in, int burn, int n_loops, int pause_at) {
    hwtimer_t tmr = hwtimer_alloc();
    
    if(burn){
        local_thread_mode_set_bits(thread_mode_fast); // Always issue
    }

    int n_sd_loops = 5; // How many samples to pass in each loop

    const int num_buffs_in_sd = 4;

    int32_t data[num_buffs_in_sd][SDAC_BUF_TOTAL] = {{0}};
    int sample_idx = 0;

    for(int loop_count = 0; loop_count < n_loops; loop_count++){
        // printintln(loop_count);
        int idx = loop_count % num_buffs_in_sd;
        
        // Send to SD modulator/PWM
        // First setup array to transfer to SD
        data[idx][SDAC_BUF_N] = n_sd_loops;
        for(int i = 0; i < n_sd_loops; i++){
            int32_t sample = sin1500[sample_idx % 1500] >> 2; // More amplitude than this and it clips/ overflows
            data[idx][SDAC_BUF_L + i] = sample;
            data[idx][SDAC_BUF_R + i] = -sample;
            sample_idx++;
        }

        if(loop_count == pause_at){
            const int num_milliseconds_pause = 20;
            printstr("pause\n");
            hwtimer_delay(tmr, XS1_TIMER_KHZ * num_milliseconds_pause);
        }

        chanend_out_word(c_sd_in, (int) &data[idx][0]);

        if(loop_count == 0){
            sd->timeout_occurred = 0; // Ensure we start clean after startup
        }

    }
    running = 0; // Stop consuming samples in consumer app

    printf("Timeout: %d\n", sd->timeout_occurred);
    printf("Completed test app\n");
    hwtimer_delay(tmr, XS1_TIMER_MHZ); // FLush print

    _Exit(0);
}

DECLARE_JOB(burn, (void));
void burn(void){
    while(1);
}

// This ensures we don't start the SD until we are ready to produce
// Ensures we don't hit the timeout case in SD at startup
DECLARE_JOB(sigma_delta_task_sf_delay_start, (sw_dac_sf_t *, chanend_t));
void sigma_delta_task_sf_delay_start(sw_dac_sf_t * sd, chanend_t c_sd){
    hwtimer_t tmr = hwtimer_alloc();
    hwtimer_delay(tmr, 20 * 100); // 20us
    hwtimer_free(tmr);
    sigma_delta_task_sf(sd, c_sd);
}

int main(int argc, char *argv[]) {
    if(argc != 4){
        printf("Error - need to pass burn and loops and pause_at as args\n");
        _Exit(-1);
    }
    int burn = atoi(argv[1]);
    int n_loops = atoi(argv[2]);
    int pause_at = atoi(argv[3]);

    printf("Started test app, burn: %d, n_loops: %d, pause_at: %d\n", burn, n_loops, pause_at);

    channel_t c_sd_ip = chan_alloc();
    channel_t c_sd_op_0 = chan_alloc();
    channel_t c_sd_op_1 = chan_alloc();
    xclock_t clk = XS1_CLKBLK_1;
#if SW_DAC_SD_TEST_MODE
    port_t ports[2] = {c_sd_op_0.end_a, c_sd_op_1.end_a};   // L and R outputs
#else
    port_t ports[2] = {XS1_PORT_1A, XS1_PORT_1B};   // L and R outputs
    port_enable(ports[0]);
    port_enable(ports[1]);
#endif
    // Setup clock block to run from MCLK in which is set to 24MHz by the App PLL
    clock_enable(clk);
    clock_set_source_clk_ref(clk);
    clock_set_divide(clk, 4 / 2); // 25MHz
    
    sw_dac_sf_t sd;
    sw_dac_sf_init(&sd, ports, clk, 8, sd_coeffs_o6_f1_5_n8,
                    2.8544, 2.8684735298,      // scale, limit
                    1.0/120000, -1.0/250000,   // flat_comp_x2, x3
                    3.0/157, 0.63/157);        // pwm comp x2, x3

    // For xscope runs, we cannot sustain 2 x 1.5MHz 32b streams so make timeout large
    if(n_loops == pause_at){
        sd.timeout_period = 100000;
    }


    if(burn){
    PAR_JOBS(
        PJOB(burn,                              ()),
        PJOB(burn,                              ()),
        PJOB(burn,                              ()),
        PJOB(test_producer,                     (&sd, c_sd_ip.end_a, burn, n_loops, pause_at)),
        PJOB(test_consumer,                     (c_sd_op_0.end_b, c_sd_op_1.end_b, burn)),
        PJOB(sigma_delta_task_sf_delay_start,   (&sd, c_sd_ip.end_b))
        );
    } else {
    PAR_JOBS(
        PJOB(test_producer,                     (&sd, c_sd_ip.end_a, burn, n_loops, pause_at)),
        PJOB(test_consumer,                     (c_sd_op_0.end_b, c_sd_op_1.end_b, burn)),
        PJOB(sigma_delta_task_sf_delay_start,   (&sd, c_sd_ip.end_b))
        );

    }
}
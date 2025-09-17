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

const int n_sd_loops = 5;


DECLARE_JOB(test_app, (sw_dac_sf_t *, chanend_t, int, int));
void test_app(sw_dac_sf_t *sd, chanend_t c_sd_in, int burn, int n_loops) {
    xscope_mode_lossless();
    
    if(burn){
        local_thread_mode_set_bits(thread_mode_fast); // Always issue
    }

    const int num_buffs_in_sd = 4;

    int32_t data[num_buffs_in_sd][SDAC_BUF_TOTAL] = {{0}};
    int sample_idx = 0;

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

const int nominal_delay = 100;

DECLARE_JOB(select_test, (chanend_t, hwtimer_t));
void select_test(chanend_t c, hwtimer_t tmr)
{
    int time_trig = hwtimer_get_time(tmr);
    hwtimer_set_trigger_time(tmr, time_trig + nominal_delay);
    int last_chan = 0;

    port_t p_out = XS1_PORT_16A;
    port_enable(p_out);
    int port_val = 0x55555555;

    // Normal loop - receive word over chan but timeout if takes too long
    SELECT_RES(
        CASE_THEN(c, event_c),
        CASE_THEN(tmr, event_tmr))
    {
        // Normal loop
        event_c:
        {
            last_chan = chanend_in_word(c);
            time_trig = hwtimer_get_time(tmr);
            time_trig += nominal_delay;
            hwtimer_change_trigger_time(tmr, time_trig);
            port_out(p_out, last_chan);
            SELECT_CONTINUE_NO_RESET;
        }

        // Timeout
        event_tmr:
        {
#if 1 // Single select - simpler
            // Send idle word on port immediately
            port_out(p_out, port_val);
            port_val = ~port_val; // There is a bit of a gap if always idle so ensure we have no DC on output
            time_trig = hwtimer_get_time(tmr);
            time_trig += nominal_delay;
            hwtimer_change_trigger_time(tmr, time_trig);
            SELECT_CONTINUE_NO_RESET;
#else
            // New select - send idle word until we get new value over chan
            // Send idle word on port immediately
            port_out(p_out, 0x55555555);
            // New select - send idle word until we get new value over chan
            SELECT_RES(
                CASE_THEN(c, event_restart_c),
                DEFAULT_THEN(default_idle))
            {
                // Send idle on port
                default_idle:
                port_out(p_out, 0x55555555);
                continue;
            }
            
            {
                // We have a new word over chan
                event_restart_c:
                // Do not input value, save for outer select when we break to that
                printintln(last_chan);
                time_trig = hwtimer_get_time(tmr);
                time_trig += nominal_delay;
                hwtimer_change_trigger_time(tmr, time_trig);
                SELECT_CONTINUE_RESET;
                break;
            }
#endif
        } // event_tmr
    }
}

DECLARE_JOB(test_select, (sw_dac_sf_t *, chanend_t));
void test_select(sw_dac_sf_t *sd, chanend_t c){
    hwtimer_t tmr = hwtimer_alloc();
    int time_trig = hwtimer_get_time(tmr);
    // Increase delay until we hit threshold
    for(int i = nominal_delay - 50; i < nominal_delay + 100; i++){
        chanend_out_word(c, i);
        time_trig += i;
        hwtimer_wait_until(tmr, time_trig);
    }
    // Big delay
    time_trig += nominal_delay * 20;
    hwtimer_wait_until(tmr, time_trig);
    // Run normally
    for(int i = 0; i < 50; i++){
        chanend_out_word(c, i);
        time_trig += i;
        hwtimer_wait_until(tmr, time_trig);
    }
    _Exit(0);
}

extern void sigma_delta_1_5_test(sw_dac_sf_t *sd, chanend_t ce); // does not return

DECLARE_JOB(run_sd_pwm, (sw_dac_sf_t *, chanend_t));
void run_sd_pwm(sw_dac_sf_t *sd, chanend_t c_in) {
    hwtimer_t tmr = hwtimer_alloc();
    sd->timeout_period = 80 * n_sd_loops; // at 1.5MHz this should be 66.666 so set above
    sd->timeout_word = 0x0ff0;
    sd->timeout_resid = tmr;
    sd->timeout_occurred = 0;
    hwtimer_set_trigger_time(sd->timeout_resid, hwtimer_get_time(tmr) + sd->timeout_period);

    sigma_delta_1_5_test(sd, c_in);
}


#if 1
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
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(burn,              ()),
        PJOB(test_app,          (&sd, c_sd_ip.end_a, burn, n_loops)),
        PJOB(run_sd_pwm,        (&sd, c_sd_ip.end_b))
        );
    } else {
    PAR_JOBS(
        PJOB(test_app,          (&sd, c_sd_ip.end_a, burn, n_loops)),
        PJOB(run_sd_pwm,        (&sd, c_sd_ip.end_b))
        );

    }
}
#else
int main(void){
    hwtimer_t tmr = hwtimer_alloc();
    channel_t c = chan_alloc();

    printf("chan resID: 0x%x, tmr resID: 0x%x\n", c.end_b, tmr);

    PAR_JOBS(
        PJOB(select_test,   (c.end_b, tmr)),
        PJOB(test_select,   (c.end_a))
        );

}
#endif
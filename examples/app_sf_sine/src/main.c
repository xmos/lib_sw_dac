// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/port.h>
#include <xcore/channel.h>
#include <xs1.h>
#include <stdio.h>
#include "sw_dac.h"
#include "sw_pll.h"


#include "../../tests/common/sin768.h"
#define SINE_TABLE                              sin768
#define SAMPLE_RATE                             48000

// App PLL setup
// Found solution: IN 24.000MHz, OUT 24.000000MHz, VCO 3456.00MHz, RD  1, FD  144, FRAC 0.000 (m =   0, n =   0), OD  6, FOD    6, ERR 0.0ppm
#define APP_PLL_CTL_REG                         0x0A808F00  
#define APP_PLL_DIV_REG                         0x80000005
#define APP_PLL_FRAC_REG                        0x00000000


void main_tile_0(chanend_t c_sd) {
    puts("Started sine wave producer");
    const unsigned two_sample_periods_us = 2 * 1000000 / SAMPLE_RATE;

    port_t p_leds = XS1_PORT_4C;
    port_enable(p_leds);

    chanend_in_word(c_sd); // Wait for ready from DAC side
    watchdog_init(two_sample_periods_us);
   
    chanend_out_control_token(c_sd, 1);
    chanend_out_word(c_sd, SAMPLE_RATE);

    for(int i = 0; ; i++) {
        int sine = SINE_TABLE[i % (sizeof(SINE_TABLE) / sizeof(SINE_TABLE[0]))];
        chanend_out_word(c_sd, sine);    // Left
        chanend_out_word(c_sd, sine);    // Right

        watchdog_reset_counter(two_sample_periods_us);

        port_out(p_leds, i >> 12);      // Show LED binary counter (upper bits)
    }
}


void main_tile_1(chanend_t c_sd) {
    sw_dac_sf_t sd;
    xclock_t clk = XS1_CLKBLK_1;
    port_t ports[2] = {XS1_PORT_1M, XS1_PORT_1O};   // L and R outputs
    port_t clk_in = XS1_PORT_1D;                    // 24MHz clock in from App PLL

    // Setup clock block to run from MCLK in which is set to 24MHz by the App PLL
    port_enable(clk_in);
    clock_enable(clk);
    clock_set_source_port(clk, clk_in);
    
    // Safely set App PLL and clear fractional enable as we don't need that
    sw_pll_app_pll_init(get_local_tile_id(), APP_PLL_CTL_REG, APP_PLL_DIV_REG, APP_PLL_FRAC_REG);
    write_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_REG);


    sw_dac_sf_init(&sd, ports, clk, 8, sd_coeffs_o6_f1_5_n8,
                    2.8544, 2.8684735298,      // scale, limit
                    1.0/120000, -1.0/250000,   // flat_comp_x2, x3
                    3.0/157, 0.63/157);        // pwm comp x2, x3

    chanend_out_word(c_sd, 0); // Send word to say ready because PLL setup takes time

    sw_dac_sf(&sd, c_sd);
}

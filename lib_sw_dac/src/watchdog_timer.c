// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <xs1.h>
#include "watchdog_timer.h"

void watchdog_reset_counter(unsigned microseconds){
    
    write_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_WATCHDOG_COUNT_NUM, microseconds);
}

void watchdog_init(unsigned microseconds){
    // These are 16b registers
    write_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_WATCHDOG_CFG_NUM, 0x0); // disable counter and resest function
    watchdog_reset_counter(microseconds);
    write_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_WATCHDOG_PRESCALER_WRAP_NUM, PLATFORM_OSCILLATOR_FREQUENCY_HZ / 1000000 - 1); // 1 MHz (1us)
    write_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_WATCHDOG_CFG_NUM, 0x3); // enable counter and resest function
}

int watchdog_has_tiggered(void){
    unsigned status = 0;
    read_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_WATCHDOG_STATUS_NUM, &status);
    
    return (status & 0x1);
}
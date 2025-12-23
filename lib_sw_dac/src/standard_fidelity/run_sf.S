// Copyright 2024-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
    
    .issue_mode dual

#include "sdac_sf.h"

#define NSTACKWORDS     20
#define FUNCTION_NAME sigma_delta_1_5

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function


    // sigma_delta_1_5(sw_dac_sf_t *sdac, chanend_t *chan)
    
    // Scratch registers:    r2 r3 r11

    // TODO: use DP rather than r10; frees up r10

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, NSTACKWORDS
    .align 16
FUNCTION_NAME:
    { dualentsp NSTACKWORDS              ; nop}
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]
    stw r10, sp[6]
    stw dp, sp[7]

    // Now store select info for the timeout and copy to stack for easier access
    stw r0, sp[SDAC_STACK_SD_STRUCT_BASE] // Store sd struct addr
    ldw r4, r0[SDAC_TIMEOUT_PERIOD]
    stw r4, sp[SDAC_STACK_TIMEOUT_PERIOD]
    ldw r4, r0[SDAC_TIMEOUT_RESID]
    stw r4, sp[SDAC_STACK_TIMEOUT_RESID]
    ldw r4, r0[SDAC_TIMEOUT_WORD]
    stw r4, sp[SDAC_STACK_TIMEOUT_WORD]
    ldw r4, r0[SDAC_CLOCK_BLOCK]
    stw r4, sp[SDAC_STACK_TIMEOUT_CLKBLK]
    

    // Initialise registers for sigma_delta_1_5
    ldw  r10, r0[SDAC_PWM_LOOKUP]
    ldw  r4, r0[SDAC_SD_COEFFS]
    ldw  r5, r0[SDAC_SD_STATE_L]
    ldw  r2, r0[SDAC_SD_STATE_R]
    ldc  r3, 32
    ldw  r7, r0[SDAC_OUT_PORTS_L]
    ldw  r8, r0[SDAC_OUT_PORTS_R]
    
    // Fill the ports with data, but clock not started yet
    ldw r11, r0[SDAC_TIMEOUT_WORD]
    out res[r7], r11
    out res[r8], r11

    // Now start the clock block, it is on already and linked to the ports
    ldw r11, r0[SDAC_CLOCK_BLOCK]
    setc res[r11], XS1_SETC_RUN_STARTR

    // Finally initialise channel register for sigma_delta
    add  r0, r1, 0
    
    bl  sigma_delta_1_5_bespoke_abi
    
.exit2:
    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldd r8, r9, sp[2]
    ldw r10, sp[6]
    ldw dp, sp[7]
    retsp NSTACKWORDS
    
    .cc_bottom FUNCTION_NAME.function

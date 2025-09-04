// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual
    
#define NSTACKWORDS     16
#define FUNCTION_NAME filter_x2_i4_o8_n16

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Input  r1
    // Output r0
    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, NSTACKWORDS
    .align 16
FUNCTION_NAME:
    { dualentsp NSTACKWORDS              ; nop}
    stw r10, sp[1]
    std r4, r5, sp[1]
    std r6, r7, sp[2]
    std r8, r9, sp[3]

    add  r8, r1, 0
    add  r7, r0, 0
    
    bl filter_x2_i4_o8_n16_bespoke_abi
    
    ldw r10, sp[1]
    ldd r4, r5, sp[1]
    ldd r6, r7, sp[2]
    ldd r8, r9, sp[3]
    
    retsp NSTACKWORDS
    .cc_bottom FUNCTION_NAME.function

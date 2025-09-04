// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

#define FUNCTION_NAME filter_x2_i1_o2_n16

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Scratch registers:    r3 r11     modified
    // Filter:               r2;        modified
    // Input:                r1;        modified
    // Output:               r0;        not modified

    // Must be called in dual issue mode
    // Must have space on stack words 10..17
    
#define NSTACKWORDS     8
    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, NSTACKWORDS
    .align 16
FUNCTION_NAME:
    { dualentsp NSTACKWORDS              ; nop}

    { vclrdr                    ; ldc r3, 32 }

    { vldc r1[0]                ; sub r11, r1, 4 }
    { vstc r11[0]               ; add r1, r1, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }

    { vstr r0[0]                ; nop }
    
    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

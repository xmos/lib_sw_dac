// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

#define FUNCTION_NAME filter_x2_i1_o2_n32

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

#define ONE 0x40000000
firsts: 
    .word 0
seconds:    
    .word ONE,0,ONE,0,ONE,0,ONE, 0

    // Scratch registers:    r2 r3 r11  modified
    // Input:                r5;        modified
    // Output:               r4;        not modified

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

#define INNER_PROD_TWO_COEFFS \
    { vldc r1[0]                ; sub r11, r1, 4 }; \
    { vstc r11[0]               ; add r1, r1, r3 }; \
    { vlmaccr r2[0]             ; add r2, r2, r3 }; \
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    
    INNER_PROD_TWO_COEFFS
    INNER_PROD_TWO_COEFFS
    
    ldaw r3, sp[0]
    
    { vstr r3[0]                     ; nop }
    { vldc r3[0]                     ; nop }
    { vclrdr                         ; ldap r11, firsts }
    { vlmaccr r11[0]                 ; ldap r11, seconds }
    { vlmaccr r11[0]                 ; nop }
    { vstr r0[0]                     ; nop }
    
    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

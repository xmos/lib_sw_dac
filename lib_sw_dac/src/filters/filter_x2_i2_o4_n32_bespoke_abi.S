// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

#define FUNCTION_NAME filter_x2_i2_o4_n32_bespoke_abi

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

#define NSTACKWORDS 0

#define ONE 0x40000000

firsts_4x: 
    .word 0
seconds_4x:    
    .word 0
thirds_4x:    
    .word 0
fourths_4x:    
    .word ONE,0,0,0,ONE,0,0,0

    // Scratch registers:    r2 r3 r11  modified
    // Input:                r4;        modified
    // Output:               r1;        not modified

    // Must be called in dual issue mode
    // Must have space on stack words 10..17
    
    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, NSTACKWORDS
    .align 16
FUNCTION_NAME:
    
    { vclrdr                    ; ldc r3, 32 }

#define INNER_PROD_FOUR_COEFFS \
    { vldc r4[0]                ; sub r11, r4, 8 }; \
    { vstc r11[0]               ; add r4, r4, r3 }; \
    { vlmaccr r2[0]             ; add r2, r2, r3 }; \
    { vlmaccr r2[0]             ; add r2, r2, r3 }; \
    { vlmaccr r2[0]             ; add r2, r2, r3 }; \
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    
    INNER_PROD_FOUR_COEFFS
    INNER_PROD_FOUR_COEFFS
    
    ldaw r3, sp[10]
    
    { vstr r3[0]                     ; nop }
    { vldc r3[0]                     ; nop }
    { vclrdr                         ; ldap r11, firsts_4x }
    { vlmaccr r11[0]                 ; ldap r11, seconds_4x }
    { vlmaccr r11[0]                 ; ldap r11, thirds_4x }
    { vlmaccr r11[0]                 ; ldap r11, fourths_4x }
    { vlmaccr r11[0]                 ; nop }
    { vstr r1[0]                     ; nop }
    
    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

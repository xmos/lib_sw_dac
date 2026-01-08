// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

#define FUNCTION_NAME filter_x2_i8_o16_n16_bespoke_abi

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Filter coefficients   r2         +lot
    // Scratch registers:    r3 r11     modified
    // Input:                r7;        +0
    // Output:               r1;        +0

    // Must be called in dual issue mode

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .align 16
FUNCTION_NAME:
    { vclrdr                    ; add r7, r7, 4 }
    { add r7, r7, 8             ; ldc r3, 32 }

// Produce two interpolated outputs
#define INNER_PROD_EIGHT_COEFFS \
    { vldc r7[0]                ; sub r7, r7, 4 }; \
    { vlmaccr r2[0]             ; add r2, r2, r3 }; \
    { vlmaccr r2[0]             ; sub r2, r2, r3 };

// Produce  0 + 4 x 2 = 8 interpolated outputs
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    { vstr r1[0]                     ; add r1, r1, r3 }
    add r7, r7, r3

    vclrdr
// Produce  8 + 4 x 2 = 16 interpolated outputs
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    { vstr r1[0]                     ; ldc r11, 20 }
    { add r7, r7, r11                ; sub r1, r1, r3 }
// copy input 8 places down
    { vldd  r7[0]                    ; sub r7, r7, r3 }
    { vstd  r7[0]                    ; nop            }
    
    retsp 0

    .cc_bottom FUNCTION_NAME.function

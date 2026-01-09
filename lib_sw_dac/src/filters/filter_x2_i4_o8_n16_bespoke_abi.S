// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

#define FUNCTION_NAME filter_x2_i4_o8_n16_bespoke_abi

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Filter coefficients   r2         +lot
    // Scratch registers:    r3 r11     modified
    // Input:                r8;        +0
    // Output:               r7;        +0

    // Must be called in dual issue mode

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .align 16
FUNCTION_NAME:
    { vclrdr                    ; add r8, r8, 4 }
    { add r8, r8, 8             ; ldc r3, 32 }

// Produce two interpolated outputs
#define INNER_PROD_EIGHT_COEFFS \
    { vldc r8[0]                ; sub r8, r8, 4 }; \
    { vlmaccr r2[0]             ; add r2, r2, r3 }; \
    { vlmaccr r2[0]             ; sub r2, r2, r3 };

// Produce  4 x 2 = 8 interpolated outputs
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    INNER_PROD_EIGHT_COEFFS
    { vstr r7[0]                     ; ldc r11, 20 }

    { add r8, r8, r11                ; ldc r11, 16 }
// copy input 8 places down
    { vldd  r8[0]                    ; sub r11, r8, r11 }
    { vstd  r11[0]                   ; add r8, r8, r3   }
    { vldd  r8[0]                    ; add r11, r11, r3 }
    { vstd  r11[0]                   ; nop   }
    
    retsp 0

    .cc_bottom FUNCTION_NAME.function

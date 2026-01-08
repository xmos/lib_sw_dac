// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

    // 2x upfilter
    // Inputs 2 samples
    // Outputs 4 samples
    // Applies a 16 tap FIR
    // Because of 2x upsampling, 8 taps are applied to calculate even samples, and 8 taps for odd samples.

#define FUNCTION_NAME filter_x2_i2_o4_n16

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Scratch registers:    r3 r11     modified
    // Coefficients:         r2;        modified
    // Input:                r1;        modified
    // Output:               r0;        not modified

#define NSTACKWORDS    0
    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, NSTACKWORDS
    .align 16
FUNCTION_NAME:
    { dualentsp NSTACKWORDS     ; add r1, r1, 4}      // First operate on signal [1..8]

    { vclrdr                    ; ldc r3, 32 }

    { vldc r1[0]                ; sub r1, r1, 4  }    // Then operate on signal [0..7]
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r3 }
    { vldc r1[0]                ; nop            }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vstr r0[0]                ; add r1, r1, 8  }
    { vldc r1[0]                ; sub r11, r1, 8 }   // Then copy samples [2..9]
    { vstc r11[0]               ; nop            }   // To samples [0..7]

    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

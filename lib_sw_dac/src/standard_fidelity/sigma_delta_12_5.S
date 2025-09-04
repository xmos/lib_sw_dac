// Copyright 2024-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "sdac_sf.h"

#define FUNCTION_NAME sigma_delta_12_5_bespoke_abi

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    .issue_mode dual
    // Used in loop          r0, dp, r9
    // Not in use            r2
    // Scratch registers:    r11 r6
    // Channel independent
    //   PWM table    r10
    //   32           r3
    //   Set coeffs   r4
    // Channel dependent
    //                L    R
    //   Output port  r7   r8
    //   State        r5   r2
    //   Input data   r1   r1
    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .align 16
FUNCTION_NAME:
    { in r11, res[r0]            ; nop }
.Louter_loop:
    { set dp, r11                ; nop }
    { ldw r9, dp[SDAC_BUF_N]    ; nop }
.Linner_loop:
    { ldw r1, dp[SDAC_BUF_L]     ; sub r9, r9, 1 }

#if 0
    ldw  r11, r5[4]
    ldw  r11, r5[5]
    ldw  r11, r5[6]
    ldw  r11, r5[7]
#endif
// Left channel:
    { stw r1, r5[0]             ; ldc r11, 31 }             // Store input value into state vector
    { ld8u r1, r5[r11]          ; ldap r11, min_max_table }// Load top byte of output value 
    { vclrdr                    ; nop         } 
    { ldw  r1, r11[r1]          ; ldc r11, 27 }            // clip value to [-5..5] and quantise
    ashr r11, r1, r11                                       // Make into a fixed point value and sign extend for PWM
    { stw r1, r5[1]             ; ldc r1, 12       }             // Store feedback
    { vldc r5[0]                ; add r1, r5, r1  }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; shl r6, r3, 2 }
    { vstr r1[0]                ; sub r4, r4, r6 }
    { ldw   r1, r10[r11]        ; ldc r11, 25 }
    outpw res[r7], r1, r11
    
    { ldw r1, dp[SDAC_BUF_R]    ; nop             }
// Right channel:
    { stw r1, r2[0]             ; ldc r11, 31 }             // Store input value into state vector
    { ld8u r1, r2[r11]          ; ldap r11, min_max_table }// Load top byte of output value 
    { vclrdr                    ; nop         } 
    { ldw  r1, r11[r1]          ; ldc r11, 27 }            // clip value to [-5..5] and quantise
    ashr r11, r1, r11                                       // Make into a fixed point value and sign extend for PWM
    { stw r1, r2[1]             ; ldc r1, 12       }             // Store feedback
    { vldc r2[0]                ; add r1, r2, r1  }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; shl r6, r3, 2 }
    { vstr r1[0]                ; sub r4, r4, r6 }
    { ldw   r1, r10[r11]        ; ldc r11, 25 }
    outpw res[r8], r1, r11

    { bt  r9, .Linner_loop       ; ldaw dp, dp[SDAC_BUF_STEP] }
    { bu  .Louter_loop           ; in r11, res[r0] }
    .align 4

#define F(x) ((x)<<27)
min_max_table:
    .word  F(0), F(0), F(0), F(0), F(0), F(0), F(0), F(0)        // 0x00
    .word  F(1), F(1), F(1), F(1), F(1), F(1), F(1), F(1)        // 0x08
    .word  F(2), F(2), F(2), F(2), F(2), F(2), F(2), F(2)        // 0x10
    .word  F(3), F(3), F(3), F(3), F(3), F(3), F(3), F(3)        // 0x18
    .word  F(4), F(4), F(4), F(4), F(4), F(4), F(4), F(4)        // 0x20
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x28
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x30
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x38
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x40
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x48
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x50
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x58
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x60
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x68
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x70
    .word  F(5), F(5), F(5), F(5), F(5), F(5), F(5), F(5)        // 0x78
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0x80
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0x88
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0x90
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0x98
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xa0
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xa8
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xb0
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xb8
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xc0
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xc8
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xd0
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xd8
    .word  F(-4), F(-4), F(-4), F(-4), F(-4), F(-4), F(-4), F(-4)        // 0xe0
    .word  F(-3), F(-3), F(-3), F(-3), F(-3), F(-3), F(-3), F(-3)        // 0xe8
    .word  F(-2), F(-2), F(-2), F(-2), F(-2), F(-2), F(-2), F(-2)        // 0xf0
    .word  F(-1), F(-1), F(-1), F(-1), F(-1), F(-1), F(-1), F(-1)        // 0xf8

    .cc_bottom FUNCTION_NAME.function

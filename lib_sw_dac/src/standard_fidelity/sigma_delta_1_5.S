// Copyright 2024-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include "sdac_sf.h"

#define FUNCTION_NAME sigma_delta_1_5_bespoke_abi

#define FRACTIONAL_BITS              28    // TODO: modified software_dac_sf.c scale2 in sync

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    .issue_mode dual
    // Used in loop          r0, dp, r9
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
    // Init VPU
    ldc r11, 0
    vsetc r11
// setup select
    ldap r11, .sd_timeout; //r11 <- &sd_timeout handler
    ldw r6, sp[SDAC_STACK_TIMEOUT_RESID];
    setv res[r6], r11;  //set event vector
    eeu res[r6]; // enable

    ldap r11, .input_handler; //r11 <- input_handler
    setv res[r0], r11;
    eeu res[r0]; // enable EEBLE

    setsr 0x01; // event enable in thread

    waiteu; // This does the initial IN or timeout
// Main loop
.Louter_loop:
.input_handler:
    // This section inits the DP, loads nsamp and moves on the timeout timer according to nsamp
    // We have r11, r6 and also r1 is available in this stage until the inner loop
    { in r11, res[r0]           ; ldw r1, sp[SDAC_STACK_TIMEOUT_PERIOD]}    // Grab input data pointer from filter + load timeout time period
    { ldw r9, r11[SDAC_BUF_N]   ; gettime r6}                               // Get number of samples, Read timer value (we know all timers are same val)
    mul r1, r1, r9                                                          // mul timeout time by num samples to receive (inner loop count)
    { ldw r6, sp[SDAC_STACK_TIMEOUT_RESID] ; add r1, r1, r6;}               // Copy input token (sf ptr) to DP for later use & move on timer time
    { set dp, r11               ; setd res[r6], r1}                         // Move time trigger forwards, and init the DP for later use

.Linner_loop:
    { ldw r1, dp[SDAC_BUF_L]    ; sub r9, r9, 1 }

// Left channel:
    { stw r1, r5[0]             ; ldc r11, 31 }             // Store input value into state vector
    { ld8u r1, r5[r11]          ; ldap r11, min_max_table } // Load top byte of output value
    { vclrdr                    ; nop         } 
    { ldw  r1, r11[r1]          ; ldc r11, FRACTIONAL_BITS} // clip value to [-5..5] and quantise
    ashr r6, r1, r11                                        // Make into a fixed point value and sign extend for PWM
    { stw r1, r5[1]             ; ldc r1, 8       }         // Store feedback
    { vldc r5[0]                ; add r1, r5, r1  }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; ldap r11, zeroes }
    { vlsat r11[0]              ; shl r11, r3, 2 }
    { vstr r1[0]                ; sub r4, r4, r11 }
    { ldw   r1, r10[r6]         ; sub r4, r4, r3 }
#if SW_DAC_SD_TEST_MODE
    out res[r7], r1
#else
    outpw res[r7], r1, 16
#endif
    
    { ldw r1, dp[SDAC_BUF_R]    ; nop             }
// Right channel:
    { stw r1, r2[0]             ; ldc r11, 31 }             // Store input value into state vector
    { ld8u r1, r2[r11]          ; ldap r11, min_max_table } // Load top byte of output value
    { vclrdr                    ; nop         } 
    { ldw  r1, r11[r1]          ; ldc r11, FRACTIONAL_BITS} // clip value to [-5..5] and quantise
    ashr r6, r1, r11                                        // Make into a fixed point value and sign extend for PWM
    { stw r1, r2[1]             ; ldc r1, 8       }             // Store feedback
    { vldc r2[0]                ; add r1, r2, r1  }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; add r4, r4, r3 }
    { vlmaccr r4[0]             ; ldap r11, zeroes }
    { vlsat r11[0]              ; shl r11, r3, 2 }
    { vstr r1[0]                ; sub r4, r4, r11 }
    { ldw   r1, r10[r6]         ; sub r4, r4, r3 }
#if SW_DAC_SD_TEST_MODE
    out res[r8], r1
#else
    outpw res[r8], r1, 16
#endif
    
    { bt  r9, .Linner_loop       ; ldaw dp, dp[SDAC_BUF_STEP] }
    waiteu

    // Select handler for timeout. This does not need to speed optimised
    .align 8
.sd_timeout:
    // Get the out dummy word, output it and then invert it to avoid DC output content
    ldw r11, sp[SDAC_STACK_TIMEOUT_WORD]
    ldw r6, sp[SDAC_STACK_TIMEOUT_CLKBLK]
#if SW_DAC_SD_TEST_MODE
    out res[r7], r11
    out res[r8], r11
#else
    // Note this isn't perfect - the port buffer will empty before next timeout
    // However since this is a bad situation anyway (output should have been muted if expected)
    // and the timeout word is inverted so we will have continuous activity on the port and near 50% duty
    // so it will avert issues where the SD is not fed which result in toasting the amplified
    syncr res[r7]                            // Ensure we have space in port buffers 
    syncr res[r8]
    setc res[r6], XS1_SETC_RUN_STOPR         // Stop clock so we can pre-load both ports
    out res[r7], r11
    out res[r8], r11
    setc res[r6], XS1_SETC_RUN_STARTR        // Start the clock outputs will be sync'ed until empty
#endif
    not r11, r11;
    stw r11, sp[SDAC_STACK_TIMEOUT_WORD];


    // Move the timeout timer on by at least one PWM period. We may come back here if no IN from filter
    { gettime r11               ; ldw r6, sp[SDAC_STACK_TIMEOUT_PERIOD]}
    { add r11, r11, r6          ; ldw r6, sp[SDAC_STACK_TIMEOUT_RESID]}
    { setd res[r6], r11         ; ldc r11, 0x1;} // Move time trigger forwards and load timeout flag
    
    // Set timeout occurred flag in struct
    ldw r6, sp[SDAC_STACK_SD_STRUCT_BASE]   ;
    {stw r11, r6[SDAC_TIMEOUT_OCCURED]      ; waiteu}


    .align 4

#define F(x) ((x)<<(FRACTIONAL_BITS - 1))
zeroes:   // Must be before min_max_table
    .word 0,0,0,0,0,0,0,0
min_max_table:
#define ADD_HALF
#if FRACTIONAL_BITS == 27
#if !defined(ADD_HALF)                                                   // AH  !AH
    .word  F(-1), F(-1), F(-1), F(-1)                                    //      00
#endif
    .word  F( 1), F( 1), F( 1), F( 1)                                    // 0x00 04
    .word  F( 1), F( 1), F( 1), F( 1), F( 3), F( 3), F( 3), F( 3)        // 0x04 08
    .word  F( 3), F( 3), F( 3), F( 3), F( 5), F( 5), F( 5), F( 5)        // 0x0c 10
    .word  F( 5), F( 5), F( 5), F( 5), F( 7), F( 7), F( 7), F( 7)        // 0x14 18
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x1c 20
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x24 28
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x2c 30
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x34 38
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x3c 40
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x44 48
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x4c 50
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x54 58
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x5c 60
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x64 68
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x6c 70
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x74 78
#if defined(ADD_HALF)
    .word  F( 7), F( 7), F( 7), F( 7)                                    // 0x7c
#else
    .word  F(-7), F(-7), F(-7), F(-7)                                    //      80
#endif
    .word                              F(-7), F(-7), F(-7), F(-7)        // 0x80 84
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0x84 88
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0x8c 90
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0x94 98
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0x9c a0
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xa4 a8
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xac b0
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xb4 b8
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xbc c0
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xc4 c8
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xcc d0
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xd4 d8
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xdc e0
    .word  F(-7), F(-7), F(-7), F(-7), F(-5), F(-5), F(-5), F(-5)        // 0xe4 e8
    .word  F(-5), F(-5), F(-5), F(-5), F(-3), F(-3), F(-3), F(-3)        // 0xec f0
    .word  F(-3), F(-3), F(-3), F(-3), F(-1), F(-1), F(-1), F(-1)        // 0xf4 f8
#if defined(ADD_HALF)
    .word  F(-1), F(-1), F(-1), F(-1)                                    // 0xfc
#endif

#elif FRACTIONAL_BITS == 28

#if !defined(ADD_HALF)                                                   // AH  !AH
    .word  F(-1), F(-1), F(-1), F(-1), F(-1), F(-1), F(-1), F(-1)        //      00
#endif
    .word  F( 1), F( 1), F( 1), F( 1), F( 1), F( 1), F( 1), F( 1)        // 0x00 08
    .word  F( 1), F( 1), F( 1), F( 1), F( 1), F( 1), F( 1), F( 1)        // 0x08 10
    .word  F( 3), F( 3), F( 3), F( 3), F( 3), F( 3), F( 3), F( 3)        // 0x10 18
    .word  F( 3), F( 3), F( 3), F( 3), F( 3), F( 3), F( 3), F( 3)        // 0x18 20
    .word  F( 5), F( 5), F( 5), F( 5), F( 5), F( 5), F( 5), F( 5)        // 0x20 28
    .word  F( 5), F( 5), F( 5), F( 5), F( 5), F( 5), F( 5), F( 5)        // 0x28 30
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x30 38
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x38 40
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x40 48
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x48 50
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x50 58
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x58 60
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x60 68
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x68 70
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x70 78
#if defined(ADD_HALF)
    .word  F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7), F( 7)        // 0x78
#else
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        //      80
#endif
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0x80 88
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0x88 90
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0x90 98
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0x98 a0
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xa0 a8
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xa8 b0
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xb0 b8
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xb8 c0
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xc0 c8
    .word  F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7), F(-7)        // 0xc8 d0
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xd0 d8
    .word  F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5), F(-5)        // 0xd8 e0
    .word  F(-3), F(-3), F(-3), F(-3), F(-3), F(-3), F(-3), F(-3)        // 0xe0 e8
    .word  F(-3), F(-3), F(-3), F(-3), F(-3), F(-3), F(-3), F(-3)        // 0xe8 f0
    .word  F(-1), F(-1), F(-1), F(-1), F(-1), F(-1), F(-1), F(-1)        // 0xf0 f8
#if defined(ADD_HALF)
    .word  F(-1), F(-1), F(-1), F(-1), F(-1), F(-1), F(-1), F(-1)        // 0xf8
#endif
#else    
#error "FRACTIONAL_BITS not defined correctly"
#endif
    .cc_bottom FUNCTION_NAME.function

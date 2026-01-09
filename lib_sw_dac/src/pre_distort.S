// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

#define NSTACKWORDS   24
#define FUNCTION_NAME pre_distort

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // r0: out
    // r1: x (& 1 history)
    // r2: pwm_comp (& 2 history)
    // r3: flat_comp (& 1 history)
    // arg4: an array of 32 coefficients: px3, px2, fx2, fx3
    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, NSTACKWORDS
    .align 16
FUNCTION_NAME:
    { dualentsp NSTACKWORDS       ; add r11, r1, 0    }
    std r4, r5, sp[0]
    std r6, r7, sp[1]
    std r8, r9, sp[2]
    { ldc   r9, 32                ; ldw r5, sp[NSTACKWORDS+2] }
    { ldw r7, sp[NSTACKWORDS+1]   ; add r6, r5, 7 }
    { ldw r4, sp[NSTACKWORDS+3]   ; shr r6, r6, 3 }
    { stw r10, sp[6]              ; shl r10, r6, 5 }
loop_4x:
    { vldr  r11[0]                ; nop               } // x
    { vlmul r1[0]                 ; ldaw r11, sp[8]   } // x
    { vstr  r11[0]                ; nop               } // x_squared
    { vlmul r1[0]                 ; add r11, r11, r9  } // x
    { vstr  r11[0]                ; sub r8, r11, r9   } // x_cubed
    { vclrdr                      ; nop               }
    { vldc  r11[0]                ; nop               } // x_cubed
    { vlmacc r7[0]                ; add r11, r7, r9    }  // comp_pwm_x3
    { vldc  r8[0]                 ; nop               }  // x_squared
    { vlmacc r11[0]               ; add r11, r11, r9  }  // comp_pwm_x2
    { vstr  r2[0]                 ; nop             }  // pwm_comp
    { vclrdr                      ; nop             }
    { vldc r8[0]                  ; add r8, r8, r9  }  // x_squared
    { vlmacc r11[0]               ; add r11, r11, r9  }  // comp_x2
    { vldc  r8[0]                 ; nop             }  // x_cubed
    { vlmacc r11[0]               ; nop             }  // comp_x3
    { vstr  r3[0]                 ; sub r11, r2, 4  }  // flat_comp

    { vldr  r11[0]                ; sub r11, r11, 4 } // pwm_comp[-1]
    { vlsub r11[0]                ; add r11, r11, 8 } // pwm_comp[-2]
    { vladd r11[0]                ; sub r11, r11, 4 } // pwm_comp[-0]
    { vlsub r11[0]                ; sub r11, r1, 4  }  // pwm_comp[-1]
    { vladd r11[0]                ; sub r11, r3, 4  }  // x[-1] 
    { vladd r11[0]                ; nop             }  // flat_comp[-1]
    // now clip and scale
    
    { vlmul r4[0]                 ; add r4, r4, r9  } // scale1
    { vlmul r4[0]                 ; sub r4, r4, r9  } // scale2 
    
    { vstr  r0[0]                 ; add r11, r1, r9  }  // o[0]

    { add r0, r0, r9              ; add r1, r1, r9 }
    { sub r6, r6, 1               ; add r3, r3, r9 }
    { add r2, r2, r9              ; bt r6, loop_4x }

    { ldw r10, sp[6]              ; add r6, r10, 4 } // r6 = 4 * ((n+7)&~7) + 4
    { nop                         ; sub      r1, r1, r6 }
    { ldw      r11, r1[r5]        ; nop }                  // x[-1]         = x[n-1]
    { stw      r11, r1[0]         ; sub      r2, r2, r6 }
    { ldw      r11, r2[r5]        ; nop }                  // pwm_comp[-1]  = pwm_comp[n-1]
    { stw      r11, r2[0]         ; sub      r2, r2, 4 }
    { ldw      r11, r2[r5]        ; nop }                  // pwm_comp[-2]  = pwm_comp[n-2]
    { stw      r11, r2[0]         ; sub      r3, r3, r6 }
    { ldw      r11, r3[r5]        ; nop }                  // flat_comp[-1] = flat_comp[n-1]
    { stw      r11, r3[0]         ; nop }
    
    ldd r4, r5, sp[0]
    ldd r6, r7, sp[1]
    ldd r8, r9, sp[2]
    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

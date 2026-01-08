// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

#define GROUP_NAME filter_x5_2_n16
.weak       _fptrgroup.GROUP_NAME.group
.weak       _fptrgroup.GROUP_NAME.nstackwords.group
.weak       _fptrgroup.GROUP_NAME.maxcores.group
.weak       _fptrgroup.GROUP_NAME.maxtimers.group
.weak       _fptrgroup.GROUP_NAME.maxchanends.group

    // 2.5 x upfilter
    // Inputs 16 samples
    // Outputs 40 samples
    // Applies a 40 tap FIR

    // Input signal is at 705,600 Hz
    // Intermediate frequency is at 3,528,000 Hz, with first signal at 685,600 Hz
    // A 39 tap filter (40 including one zero) will give us a -150 dB drop
    
    // Applies a 5x upsample, and 2x downsample:
    //  0   0   0   0   i0  0   0   0   0   i1  0   0   0   0   i2  0   0   0   0   i3  ...
    //  f0  f1  f2  f3  f4  f5  f6  f7  f8  f9  f10 f11 f12 f13 f14 f15 f16 f17 f18 f19 ...
    //          f0  f1  f2  f3  f4  f5  f6  f7  f8  f9  f10 f11 f12 f13 f14 f15 f16 f17 ...
    //                  f0  f1  f2  f3  f4  f5  f6  f7  f8  f9  f10 f11 f12 f13 f14 f15 ...
    //                          f0  f1  f2  f3  f4  f5  f6  f7  f8  f9  f10 f11 f12 f13 ...
    //                                  f0  f1  f2  f3  f4  f5  f6  f7  f8  f9  f10 f11 ... 
    //
    //  Load input data
    //  phase_4: f4, f9, f14, f19, f24, ... f39, 
    //  phase_2: f2, f7, f12, f17, f22, ... f37,
    //  phase_0: f0, f5, f10, f15, f20, .. f35,
    //  advance input
    //  Load input data
    //  phase_3: f3, f8, f13, f18, f23, ... f38, 
    //  phase_1: f1, f6, f11, f16, f21, ... f36,
    //  advance input
    //
    //  This needs repeating 8 times. Given the rotating accumulator is
    //  8 deep it needs to unrolled completely with vstr/vclrdr interspersed five times.
    //
    // Given that the accumulator is reversed we need to calculate

    // ld 2   0 2 4   ld 1  1 3      ld 0   0 2 4
    // ld 6   4       ld 5  1 3      ld 4   0 2 4   ld 3  1 3
    // ld 9   3       ld 8   0 2 4   ld 7   1 3     ld 6   0 2 
    // ld 12  2 4     ld 11  1 3     ld 10  0 2 4   ld 9   1
    // ld 15  1 3     ld 14  0 2 4   ld 13  1 3     ld 12  0

#define FUNCTION_NAME filter_x5_2_i16_o40_n40

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME

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
    { dualentsp NSTACKWORDS     ; ldc r3, 32 }    

    // 2, 2, 2, 1, 1, 0, 0, 0
    { vclrdr                    ; shl r11, r3, 2 }    // r11 = 32 * 4
    { add r1, r1, 8             ; nop }

    { vldc r1[0]                ; sub r1, r1, 4  }    // phases 0..4
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 0 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }
    { vldc r1[0]                ; ldc r11, 24    }    // r1 now points 0 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; nop            }    // Keep phase 4
    
    { vstr r0[0]                ; add r0, r0, r3  }  // Store 0..7
    { vclrdr                    ; add r1, r1, r11 }
    shl r11, r3, 2
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 5 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 4 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }   // Set coefficients back to phase 0
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 3 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; add r1, r1, r3 }    // r1 now points 11 samples ahead   
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r1, r1, 8 }      // r1 no 9 samples ahead
    
    { vstr r0[0]                ; add r0, r0, r3  }  // Store 8..15
    { vclrdr                    ; nop }
    
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 8 samples ahead
    { vlmaccr r2[0]             ; sub r2, r2, r11 }   // Set coefficients back to phase 0
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 7 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 6 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }   // Set coefficients back to phase 0
    { vldc r1[0]                ; add r1, r1, r3  }    // r1 now points 14 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r1, r1, 8 }     // r1 now 12 samples ahead

    { vstr r0[0]                ; add r0, r0, r3  }  // Store 16..23
    vclrdr
    
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 11 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 10 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }   // Set coefficients back to phase 0
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 9 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; add r1, r1, r3 }    // r1 now points 17 samples ahead
    { vlmaccr r2[0]             ; sub r1, r1, 8  }    // r1 now points 15 sampels ahead
    
    { vstr r0[0]                ; add r0, r0, r3  }  // Store 24..31
    vclrdr
    
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 14 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }   // Set coefficients back to phase 0
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 13 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 12 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }   // Set coefficients back to phase 0
    { vldc r1[0]                ; add r1, r1, 8  }    // r1 now points 14 samples ahead
    { vlmaccr r2[0]             ; add r1, r1, 8 }     // r1 now points 16 samples ahead
    
    { vstr r0[0]                ; shl r11, r3, 1 }  // Store 32..39, set r11 to 64 (16 samples back)
    
    { vldc r1[0]                ; sub r11, r1, r11}   // Then copy input samples [16..23]
    { vstc r11[0]               ; nop            }   // To samples [0..7]

    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME

#define FUNCTION_NAME filter_x5_2_i8_o20_n40

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
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp NSTACKWORDS     ; ldc r3, 32 }    

    { vclrdr                    ; shl r11, r3, 2 }    // r11 = 32 * 4
    { add r1, r1, 8             ; nop }

    // 2, 2, 2, 1, 1, 0, 0, 0
    { vldc r1[0]                ; sub r1, r1, 4  }    // phases 0..4
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 0 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }
    { vldc r1[0]                ; ldc r11, 24    }    // r1 now points 0 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; nop            }    // Keep phase 4
    
    // 6, 5, 5, 4, 4, 4, 3, 3,
    { vstr r0[0]                ; add r0, r0, r3  }  // Store 0..7
    { vclrdr                    ; add r1, r1, r11 }
    shl r11, r3, 2
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 5 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 4 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }   // Set coefficients back to phase 0
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 3 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; add r1, r1, 8 }    // r1 now points 5 samples ahead   
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r1, r1, 8 }    // r1 now 7 samples ahead

    // 7, 7, 6, 6
    { vstr r0[0]                ; add r0, r0, r3  }  // Store 8..15
    { vclrdr                    ; sub r2, r2, r3 }
    
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 6 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }   // Set coefficients back to phase 0
    { vldc r1[0]                ; add r1, r1, 8  }    // r1 now points 8 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; nop            }

    { vstr r0[0]                ; add r0, r0, r3  }  // Store 16..23
    
    { vldc r1[0]                ; sub r11, r1, r3}   // Then copy input samples [16..23]
    { vstc r11[0]               ; nop            }   // To samples [0..7]

    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME

#define FUNCTION_NAME filter_x5_2_i4_o10_n40

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
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp NSTACKWORDS     ; ldc r3, 32 }    

    { vclrdr                    ; shl r11, r3, 2 }    // r11 = 32 * 4
    { add r1, r1, 8             ; nop }

    // 2, 2, 2, 1, 1, 0, 0, 0
    
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 1 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4  }    // r1 now points 0 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }
    { vldc r1[0]                ; add r1, r1, 8 }    // r1 now points 2 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }    // Go back to phase 3
    { vstr r0[0]                ; add r0, r0, r3 }   // Store 0..7

    // 3, 3
    { vclrdr                    ; add r1, r1, 4  }    // r1 now points 3 samples ahead
    { vldc r1[0]                ; sub r11, r1, 4 }    // r11 now points 2 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    
    { vstr r0[0]                ; add r1, r1, 4  }  // Store 8..15, r1 now points 4 samples ahead
    
    { vldc r1[0]                ; sub r11, r11, 8}   // Then copy input samples [4..11]
    { vstc r11[0]               ; nop            }   // To samples [0..7]

    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME

#define FUNCTION_NAME filter_x5_2_i2_o5_n40

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
    { dualentsp NSTACKWORDS     ; ldc r3, 32     }    

    // 1, 1, 0, 0, 0
    { vclrdr                    ; shl r11, r3, 2  }  // r11: 128
    { add r1, r1, 4             ; add r2, r2, r11 }
    sub r2, r2, r3
    { vldc r1[0]                ; sub r1, r1, 4  }    // phases 0..4
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; sub r2, r2, r11 }
    { vldc r1[0]                ; add r1, r1, 8  }    // r1 now points 2 samples ahead
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; nop            }
    
    { vstr r0[0]                ; nop            }   // Store 0..4
    
    { vldc r1[0]                ; sub r11, r1, 8 }   // Then copy input samples [2..9]
    { vstc r11[0]               ; nop            }   // To samples [0..7]

    retsp NSTACKWORDS

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME

    

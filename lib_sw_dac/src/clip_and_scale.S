// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual
    
#include "sw_dac_conf_default.h"
#define NSTACKWORDS      0
#define FUNCTION_NAME clip_and_scale

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // r0: out
    // r1: x (& 1 history)
    // r2: scale1 (clipping)
    // r3: scale2 (scaling)

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, NSTACKWORDS
    .align 16
FUNCTION_NAME:
    { dualentsp NSTACKWORDS       ; add r11, r1, 0    }
    
    { vldr  r11[0]                ; ldc r11, 32       } // x
    { vlmul r2[0]                 ; add r1, r1, r11   } // scale1
    { vlmul r3[0]                 ;                   } // scale2
    { vstr  r0[0]                 ; add r11, r1, 0    } // out

    { vldr  r11[0]                ; ldc r11, 32       } // x
    { vlmul r2[0]                 ; add r1, r1, r11   } // scale1
    { vlmul r3[0]                 ; add r0, r0, r11   } // scale2
    { vstr  r0[0]                 ; add r11, r1, 0    } // out

    { vldr  r11[0]                ; ldc r11, 32       } // x
    { vlmul r2[0]                 ; add r1, r1, r11   } // scale1
    { vlmul r3[0]                 ; add r0, r0, r11   } // scale2
    { vstr  r0[0]                 ; add r11, r1, 0    } // out

    { vldr  r11[0]                ; nop               } // x
    { vlmul r2[0]                 ; ldc r11, 32       } // scale1
    { vlmul r3[0]                 ; add r0, r0, r11   } // scale2
    { vstr  r0[0]                 ; nop               } // out

    retsp NSTACKWORDS

    // float max_value = 2.8684735298;
    // float fs_to_full_scale = 2.8544;
    // int scale1 = 0x40000000LL * fs_to_full_scale / max_value * 2;
    // int scale2 = 0x40000000LL * max_value / 8 / 2;

#define SD_SCALE1    2136947495 // 2073432488
#define SD_SCALE2_    192500000 //  194615706

#if SW_DAC_NEGATE
#define SD_SCALE2   (-(SD_SCALE2_))
#else
#define SD_SCALE2   (SD_SCALE2_)
#endif

scale_limit:
    .word     SD_SCALE1,SD_SCALE1,SD_SCALE1,SD_SCALE1,SD_SCALE1,SD_SCALE1,SD_SCALE1,SD_SCALE1
scale_factor:   
    .word     SD_SCALE2,SD_SCALE2,SD_SCALE2,SD_SCALE2,SD_SCALE2,SD_SCALE2,SD_SCALE2,SD_SCALE2
    .cc_bottom FUNCTION_NAME.function


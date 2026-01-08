// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    .issue_mode dual

#define GROUP_NAME filter_x125_64
.weak       _fptrgroup.GROUP_NAME.group
.weak       _fptrgroup.GROUP_NAME.nstackwords.group
.weak       _fptrgroup.GROUP_NAME.maxcores.group
.weak       _fptrgroup.GROUP_NAME.maxtimers.group
.weak       _fptrgroup.GROUP_NAME.maxchanends.group

#define FUNCTION_NAME filter_x125_64_i4_o8_n16_phase_0123489cde

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Filter coefficients   r2         +lot
    // Scratch registers:    r3 r11     modified
    // Input:                r1;        +0
    // Output:               r0;        +48

    // Eats 4 samples - produces 8 (phase 0, 1, 2, 3, 4,   6, 7, 8, 9,   11, 12, 13, 14) or 7 (phase 5, 10, 15)

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; ldc r3, 32 }
    { vclrdr                    ; add r1, r1, 8 }
    { nop                       ; add r1, r1, 4 }
// 3, 3, 2, 2, 1, 1, 0, 0,

    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; nop            }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; nop     }
    { vstr r0[0]                ; nop }
    { retsp 0                   ; ldc r0, 8 }

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME
#define FUNCTION_NAME filter_x125_64_i4_o8_n16_phase_67b

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Filter coefficients   r2         +lot
    // Scratch registers:    r3 r11     modified
    // Input:                r1;        +0
    // Output:               r0;        +48

    // Eats 4 samples - produces 8 (phase  6, 7)

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; ldc r3, 32 }
    { vclrdr                    ; add r1, r1, 8 }
    { nop                       ; add r1, r1, 8 }
// 4, 3, 3, 2, 2, 1, 1, 0

    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; nop            }
    { vlmaccr r2[0]             ; nop            }
    { vstr r0[0]                ; nop }
    { retsp 0                   ; ldc r0, 8 }

    .cc_bottom FUNCTION_NAME.function


#undef FUNCTION_NAME
#define FUNCTION_NAME filter_x125_64_i4_o8_n16_phase_6

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Filter coefficients   r2         +lot
    // Scratch registers:    r3 r11     modified
    // Input:                r1;        +0
    // Output:               r0;        +48

    // Eats 4 samples - produces 8 (phase  6, 7)

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; ldc r3, 32 }
    { vclrdr                    ; add r1, r1, 8 }
    { nop                       ; add r1, r1, 8 }
// 4, 3, 3, 2, 2, 1, 1, 0

    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; nop            }
    { vlmaccr r2[0]             ; nop            }
    { vstr r0[0]                ; nop }
    { retsp 0                   ; ldc r0, 8 }

    .cc_bottom FUNCTION_NAME.function


#undef FUNCTION_NAME
#define FUNCTION_NAME filter_x125_64_i4_o8_n16_phase_7b

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Filter coefficients   r2         +lot
    // Scratch registers:    r3 r11     modified
    // Input:                r1;        +0
    // Output:               r0;        +48

    // Eats 4 samples - produces 8 (phase  6, 7)

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; ldc r3, 32 }
    { vclrdr                    ; add r1, r1, 8 }
    { nop                       ; add r1, r1, 8 }
// 4, 3, 3, 2, 2, 1, 1, 0

    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; nop            }
    { vlmaccr r2[0]             ; nop            }
    { vstr r0[0]                ; nop }
    { retsp 0                   ; ldc r0, 7 }

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME
#define FUNCTION_NAME filter_x125_64_i4_o8_n16_phase_5


    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; ldc r3, 32 }
    { vclrdr                    ; add r1, r1, 8 }
    add r1, r1, 8

//  4, 3, 3, 2, 2, 1, 0, 0, 

    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; nop            }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; nop }
    { vstr r0[0]                ; nop }
    { retsp 0                   ; ldc r0, 8 }


    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME
#define FUNCTION_NAME filter_x125_64_i4_o8_n16_phase_a

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; ldc r3, 32 }
    { vclrdr                    ; add r1, r1, 8 }
    { nop                       ; add r1, r1, 8 }

// 4, 3, 3, 2, 1, 1, 0, 0
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; nop            }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; nop            }
    { vstr r0[0]                ;                }

    { retsp 0                   ; ldc r0, 8 }


    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME
#define FUNCTION_NAME filter_x125_64_i4_o8_n16_phase_f

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; add r1, r1, 4 }
    { vclrdr                    ; ldc r3, 32 }
    { nop                       ; add r1, r1, 8 }

// X, 3, 2, 2, 1, 1, 0, 0
    { vclrdr                    ; ldc r11, 16 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; sub r1, r1, 4 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vldc r1[0]                ; nop            }
    { vlmaccr r2[0]             ; add r2, r2, r3 }
    { vlmaccr r2[0]             ; ldc r3, 16     }
    { vstr r0[0]                ; add r11, r1, r3 }

    { vldd r1[0]                ; nop            }          // Shuffle down 4 words
    vstd r1[0]
    { retsp 0                   ; ldc r0, 7 }

    .cc_bottom FUNCTION_NAME.function


#undef FUNCTION_NAME
#define FUNCTION_NAME         filter_shuffle_n24_n8 // shuffle down 16 place

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Output:               r0;        samples dest

    // Eats 4 samples - produces 8 (phase 0, 1, 2, 3, 4,   6, 7, 8, 9,   11, 12, 13, 14) or 7 (phase 5, 10, 15)

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; nop }
    ldc r11, 64
    add r1, r0, r11
    vldd r1[0]
    vstd r0[0]
    retsp 0

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME
#define FUNCTION_NAME         filter_shuffle_n24_n16 // shuffle down 4 places

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Output:               r0;        samples dest

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; ldc r11, 32 }
    { add r1, r0, r11           ; ldc r3, 32 }
    { vldd r1[0]                ; add r1, r1, r3 }
    { vstd r0[0]                ; add r0, r0, r3 }
    vldd r1[0]
    vstd r0[0]
    retsp 0

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME
   
#define FUNCTION_NAME         filter_shuffle_n24_n20 // shuffle down 4 places

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Output:               r0;        samples dest

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .align 16
FUNCTION_NAME:
    { dualentsp 0               ; ldc r11, 16 }
    { add r1, r0, r11           ; ldc r3, 32 }
    { vldd r1[0]                ; add r1, r1, r3 }
    { vstd r0[0]                ; add r0, r0, r3 }
    vldd r1[0]
    vstd r0[0]
    retsp 0

    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME


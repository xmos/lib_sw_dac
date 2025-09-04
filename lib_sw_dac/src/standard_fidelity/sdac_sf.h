// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#define SDAC_SD_STATE_L  0
#define SDAC_SD_STATE_R  1

#define SDAC_PWM_LOOKUP  2

#define SDAC_CLOCK_BLOCK 3

#define SDAC_INPUT_CHANE 4

#define SDAC_BANK        5

#define SDAC_SD_COEFFS   6
#define SDAC_OUT_PORTS_L 7   // Two ports
#define SDAC_OUT_PORTS_R 8



#define SDAC_FILTER_MAX  42
#define SDAC_BUF_N       0
#define SDAC_BUF_L       1
#define SDAC_BUF_R       (SDAC_BUF_L + SDAC_FILTER_MAX)
#define SDAC_BUF_STEP    1
#define SDAC_BUF_TOTAL   (SDAC_BUF_R + SDAC_FILTER_MAX)

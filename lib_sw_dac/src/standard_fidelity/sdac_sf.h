// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

// These are offsets to the sw_dac_sf_t struct
// used from the singa-delta assembly API
// Do not modify!

#define SDAC_SD_STATE_L  0
#define SDAC_SD_STATE_R  1

#define SDAC_PWM_LOOKUP  2

#define SDAC_CLOCK_BLOCK 3

#define SDAC_SD_COEFFS   4

#define SDAC_OUT_PORTS_L 5   // Two ports
#define SDAC_OUT_PORTS_R 6
#define SDAC_TIMEOUT_TRIG     7   // Used for safe timeout in SD/PWM
#define SDAC_TIMEOUT_PERIOD   8   // Used for safe timeout in SD/PWM
#define SDAC_TIMEOUT_WORD   9   // Used for safe timeout in SD/PWM
#define SDAC_TIMEOUT_RESID   10   // Used for safe timeout in SD/PWM
#define SDAC_TIMEOUT_OCCURED   11   // Used for safe timeout in SD/PWM

// These are offsets to the array exchanged between
// the filter thread the sigma-delta thread
// Do not modify!

#define SDAC_FILTER_MAX  42
#define SDAC_BUF_N       0
#define SDAC_BUF_L       1
#define SDAC_BUF_R       (SDAC_BUF_L + SDAC_FILTER_MAX)
#define SDAC_BUF_STEP    1
#define SDAC_BUF_TOTAL   (SDAC_BUF_R + SDAC_FILTER_MAX)

// These are stack offsets for storing information for the select in SD
// DO NOT MODIFY!
#define SDAC_STACK_SD_STRUCT_BASE   8
#define SDAC_STACK_TIMEOUT_PERIOD   9
#define SDAC_STACK_TIMEOUT_TRIG     10
#define SDAC_STACK_TIMEOUT_RESID    11
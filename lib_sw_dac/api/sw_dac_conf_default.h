// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if !defined(__sw_dac_config_h__)
#define __sw_dac_config_h__

#if defined(__sw_dac_conf_h_exists__)
#include "sw_dac_conf.h"
#else
#warning "No sw_dac_conf.h, using default settings. Create an empty sw_dac_conf.h to remove this warning and use default settings"
#endif

#if !defined(SW_DAC_NEGATE)
/**
 * Boolean as to whether the signal is negated or not.
 * By default signals are passed through.
 */
#define SW_DAC_NEGATE                            0
#endif

#if !defined(SW_DAC_DC_REMOVAL_TIME_CONSTANT)
/**
 * Time constant for the DC removal. DC removal uses a decaying average with a
 * decay of ``1 - (2 ** -n)``. The default value of 14 is a cut-off frequency of around 0.2 Hz.
 * Set it to lower values to speed up DC removal.
 */
#define SW_DAC_DC_REMOVAL_TIME_CONSTANT          14
#endif

#endif

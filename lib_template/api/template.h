// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

#ifdef __template_conf_h_exists__
#include "template_conf.h"
#endif

#ifndef TEMPLATE_MEANING_OF_LIFE
#define TEMPLATE_MEANING_OF_LIFE (42)
#endif

/** Example function
 * \brief This function returns the meaning of life to the console.
 * \return The meaning of life as an integer.
 */
int template_get_meaning_of_life();

/** Exammple function.
 *
 * \brief This function prints the meaning of life to the console.
 *
 */
void template_print_meaning_of_life();

#endif // _TEMPLATE_H_

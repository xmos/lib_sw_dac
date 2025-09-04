// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "template.h"
#include <stdio.h>

int template_get_meaning_of_life()
{
    return TEMPLATE_MEANING_OF_LIFE;
}

void template_print_meaning_of_life()
{
    printf("The meaning of life, the universe, and everything is %d\n", TEMPLATE_MEANING_OF_LIFE);
}

// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <platform.h>
#include <assert.h>
#include <print.h>
#include "template.h"

int main()
{
    int mol = template_get_meaning_of_life();

    assert(mol == TEMPLATE_MEANING_OF_LIFE);

    printstr("PASS");
    return 0;
}

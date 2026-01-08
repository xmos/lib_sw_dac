// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>

extern void main_tile_0(chanend);
extern void main_tile_1(chanend);

int main(void) {
    chan  c_sd;

    par {
        on tile[0]: main_tile_0(c_sd);
        on tile[1]: main_tile_1(c_sd);
    }
    return 0;
}

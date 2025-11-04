// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdint.h>
#include "../../common/sin1500.h"
#include "pre_distort.h"
#include <xcore/assert.h>
#include <stdlib.h>

int scale_and_clip(int x, int scale, int max) {
    int out = (x * (int64_t) scale) >> 30;
    if (out > max) out = max;
    if (out < -max) out = -max;
    return out;
}

static int hpf_filter(int input, int history[]) {
    history[2] = history[1];
    history[1] = history[0];
    history[0] = input;
    return history[0] - 2*history[1] + history[2];
}

int pre_distort_ref(int x, int x_last[], int flat_comp_last[], int pwm_comp_last[]) {
    int comp_x2 = 0x040000000LL/120000; // 8947.848533
    int comp_x3 = 0x040000000LL/-250000; // -4294.967296
    int comp_pwm_x2 = 0x040000000LL * 3 / 157; // 20517359.69
    int comp_pwm_x3 = 0x040000000LL * 0.63 / 157; // 4308645.536
    // scaling
    float scale = 2.8544, limit = 2.8684735298;
    int32_t negate = 1; // no negate for now
    int32_t sc1 = 0x40000000 * scale / limit * 2; // 1.99018, 2136939503
    int32_t sc2 = 0x40000000 * limit / 4 / 2 * negate; // * 0.35856, 385000000

//    printf("%d %d %d %d\n", comp_x2, comp_x3, comp_pwm_x2, comp_pwm_x3);
    int x_squared = (x * (int64_t) x + (1<<29)) >> 30;
    int x_cubed = (x_squared * (int64_t) x + (1<<29)) >> 30;
    int pwm_comp  = ((comp_pwm_x2 * (int64_t)x_squared) + (comp_pwm_x3 * (int64_t)x_cubed) + (1<<29))>>30;
    int flat_comp = ((comp_x2     * (int64_t)x_squared) + (comp_x3     * (int64_t)x_cubed) + (1<<29))>>30;
    int pwm_comp_filt = hpf_filter(pwm_comp, pwm_comp_last);
//    printf("%9d %9d %9d   %9d %9d   ", pwm_comp, flat_comp, pwm_comp_filt, x_squared, x_cubed);

    int y = x_last[0] - pwm_comp_filt + flat_comp_last[0];
    y = scale_and_clip(y, sc1, 0x7fffffff);
    y = scale_and_clip(y, sc2, 0x7fffffff);

    x_last[0] = x;
    flat_comp_last[0] = flat_comp;
    return y;
}

int32_t x2x3_array[32] = {
    4308645, 4308645, 4308645, 4308645, 4308645, 4308645, 4308645, 4308645,
    20517360, 20517360, 20517360, 20517360, 20517360, 20517360, 20517360, 20517360,
    8947, 8947, 8947, 8947, 8947, 8947, 8947, 8947,
    -4294, -4294, -4294, -4294, -4294, -4294, -4294, -4294
};

int32_t sc_array[8] = {
    2136939503, 2136939503, 2136939503, 2136939503, 2136939503, 2136939503, 2136939503, 2136939503
    385000000, 385000000, 385000000, 385000000, 385000000, 385000000, 385000000, 385000000
};

int main(void) {
    // for the reference implementation
    int pwm_comp_last[3], x_last[1], flat_comp_last[1];
    pwm_comp_last[0] = 0;
    pwm_comp_last[1] = 0;
    pwm_comp_last[2] = 0;
    x_last[0] = 0;
    flat_comp_last[0] = 0;
    // for the actual implementation
    int32_t x_history_[33], *x_history = &x_history_[1];
    int32_t flat_comp_history_[33], *flat_comp_history = &flat_comp_history_[1];
    int32_t pwm_comp_history_[34], *pwm_comp_history = &pwm_comp_history_[2];
    int32_t y_out[32];
    x_history[-1] = 0;
    flat_comp_history[-1] = 0;
    pwm_comp_history[-1] = 0;
    pwm_comp_history[-2] = 0;
    // pretending that we just upsampled the signal from 48k
    int stepcount = 0;
    int steps[4] = {31,32,31,31};

    for(int j = 0; j < 1532; ) {
        int step = steps[stepcount];
        stepcount = (stepcount + 1) % 4;

        for(int i = 0; i < 32; i++) {
            x_history[i] = sin1500[(i+j)%1500]>>1;
        }

        pre_distort(y_out, x_history, pwm_comp_history, flat_comp_history, x2x3_array, step, sc_array);

        for(int i = 0; i < step; i++) {
            int x = sin1500[(i+j)%1500]>>1;
            int y = pre_distort_ref(x, x_last, flat_comp_last, pwm_comp_last);
            // printf("%9d %9ld  %2ld %9ld\n", y, y_out[i], y_out[i] - y, x_history[i]);
            xassert(abs(y_out[i] - y) < 2900); // experimental, about 20 bits of precision
        }

        j += step;
    }
    printf("pass\n");
    return 0;
}

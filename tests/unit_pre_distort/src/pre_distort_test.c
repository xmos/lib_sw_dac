// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdint.h>
#include "../../common/sin1500.h"
#include "pre_distort.h"
#include "clip_and_scale.h"

int scale_and_clip(int x, int scale, int max) {
    int out = (x * (int64_t) scale) >> 30;
    if (out > max) out = max;
    if (out < -max) out = -max;
    return out;
}

int clip_and_scale_ref(int x, int scale1, int scale2) {
    int out = ((x>>1) * (int64_t) scale1) >> 30;
    if (out > 0x3fffffff) out = 0x3fffffff;
    if (out < -0x3fffffff) out = -0x3fffffff;
    out = out << 1;
    return (out * (int64_t) scale2) >> 30;
}

int main_cs(void) {
    int32_t signal[8] = {0, 0x10000000, 0x41000000, -0x41000000, -0x10000000, 0x60000000, -0x60000000, 0};

    float max_value = 2.9;
    float fs_to_full_scale = 2.8;
    int scale = 0x40000000LL * fs_to_full_scale / 8;
    int clip = 0x40000000LL * max_value / 8;
    int scale1 = 0x40000000LL * fs_to_full_scale / max_value * 2;
    int scale2 = 0x40000000LL * max_value / 8 / 2;
    int32_t scale1_vector[8] = {scale1,scale1,scale1,scale1,scale1,scale1,scale1,scale1};
    int32_t scale2_vector[8] = {scale2,scale2,scale2,scale2,scale2,scale2,scale2,scale2};
    int32_t output[8];
    clip_and_scale(output, signal, scale1_vector, scale2_vector);
    for(int i = 0; i < 8; i++) {
        int o0 = scale_and_clip(signal[i], scale, clip);
        int o1 = clip_and_scale_ref(signal[i], scale1, scale2);
        printf("%10d %10d %10d %10d %f %f\n", signal[i], o0, o1, output[i], o0 / (float)(1<<27), o1 / (float)(1<<27));
    }
    return 0;
}

static int hpf_filter(int input, int history[]) {
    history[2] = history[1];
    history[1] = history[0];
    history[0] = input;
    return history[0] - 2*history[1] + history[2];
}

int pre_distort_ref(int x, int x_last[], int flat_comp_last[], int f_history[]) {
    int comp_x2 = 0x040000000LL/120000;
    int comp_x3 = 0x040000000LL/-250000;
    int comp_pwm_x2 = 0x040000000LL * 3 / 157;
    int comp_pwm_x3 = 0x040000000LL * 0.63 / 157;

//    printf("%d %d %d %d\n", comp_x2, comp_x3, comp_pwm_x2, comp_pwm_x3);
    int x_squared = (x * (int64_t) x + (1<<29)) >> 30;
    int x_cubed = (x_squared * (int64_t) x + (1<<29)) >> 30;
    int pwm_comp  = ((comp_pwm_x2 * (int64_t)x_squared) + (comp_pwm_x3 * (int64_t)x_cubed) + (1<<29))>>30;
    int flat_comp = ((comp_x2     * (int64_t)x_squared) + (comp_x3     * (int64_t)x_cubed) + (1<<29))>>30;
    int pwm_comp_filt = hpf_filter(pwm_comp, f_history);
//    printf("%9d %9d %9d   %9d %9d   ", pwm_comp, flat_comp, pwm_comp_filt, x_squared, x_cubed);
    int y = x_last[0] - pwm_comp_filt + flat_comp_last[0];
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

int main_d(void) {
    int history[3], x_last[1], flat_comp_last[1];
    history[0] = 0;
    history[1] = 0;
    history[2] = 0;
    x_last[0] = 0;
    flat_comp_last[0] = 0;
    int32_t x_history_[33], *x_history = &x_history_[1];
    int32_t flat_comp_history_[33], *flat_comp_history = &flat_comp_history_[1];
    int32_t pwm_comp_history_[34], *pwm_comp_history = &pwm_comp_history_[2];
    int32_t y_out[32];
    x_history[-1] = 0;
    flat_comp_history[-1] = 0;
    pwm_comp_history[-1] = 0;
    pwm_comp_history[-2] = 0;
    int stepcount = 0;
    int steps[4] = {31,32,31,31};
    for(int j = 0; j < 1532; ) {
        int step = steps[stepcount];
        stepcount = (stepcount + 1) % 4;
        for(int i = 0; i < 32; i++) {
            x_history[i] = sin1500[(i+j)%1500]>>1;
        }
        pre_distort(y_out, x_history, pwm_comp_history, flat_comp_history, &x2x3_array[0]);
        for(int i = 0; i < step; i++) {
            int x = sin1500[(i+j)%1500]>>1;
            int y = pre_distort_ref(x, x_last, flat_comp_last, history);
            printf("%9d %9ld  %2ld %9ld\n", y, y_out[i], y_out[i] - y, x_history[i]);
        }
        x_history[-1] = x_history[step-1];
        flat_comp_history[-1] = flat_comp_history[step-1];
        pwm_comp_history[-1]  = pwm_comp_history[step-1];
        pwm_comp_history[-2]  = pwm_comp_history[step-2];
        j += step;
    }
    return 0;
}

int main(void) {
    main_d();
    return 0;
}

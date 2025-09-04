// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "software_dac_sf.h"

FILE * _fopen(char * fname, char* mode) {
  FILE * fp = fopen(fname, mode);
  if (fp == NULL)
  {
    printf("Error opening a file\n");
    exit(1);
  }
  return fp;
}

#if FREQ == 48000
extern int filter_x125_4(software_dac_sf_t *sd, int32_t *output, int ch, int32_t sample);
#define filter_api filter_x125_4
#define mask 3

#elif FREQ == 96000
extern int filter_x125_8(software_dac_sf_t *sd, int32_t *output, int ch, int32_t sample);
#define filter_api filter_x125_8
#define mask 7

#elif FREQ == 192000
extern int filter_x125_16(software_dac_sf_t *sd, int32_t *output, int ch, int32_t sample);
#define filter_api filter_x125_16
#define mask 15

#else
#error "Unsupported frequency"
#endif

int main(void){
  software_dac_sf_t sd;
  xclock_t clk = XS1_CLKBLK_1;
  port_t ports[2] = {XS1_PORT_1M, XS1_PORT_1O};   // L and R outputs
  port_t clk_in = XS1_PORT_1D;                    // 24MHz clock in from App PLL
  port_t clk_out = XS1_PORT_1P;                   // Dummy, unconnected

  // Setup clock block to run from MCLK in which is set to 24MHz by the App PLL
  port_enable(clk_in);
  clock_enable(clk);
  clock_set_source_port(clk, clk_in);

  software_dac_sf_init(&sd, ports, clk, clk_out, 8, sd_coeffs_o6_f1_5_n8,
                         2.8544, 2.8684735298,      // scale, limit
                         1.0/120000, -1.0/250000,   // flat_comp_x2, x3
                         3.0/157, 0.63/157,         // pwm comp x2, x3
                         0);                        // negate

  int32_t in_samp, out_samp[100];

  FILE * in = _fopen("in.bin", "rb");
  FILE * out = _fopen("out.bin", "wb");

  fseek(in, 0, SEEK_END);
  int in_len = ftell(in) / sizeof(int32_t);
  fseek(in, 0, SEEK_SET);

  for(unsigned i = 0; i < in_len; i++) {
    fread(&in_samp, sizeof(int32_t), 1, in);
    int n = filter_api(&sd, out_samp, 0, in_samp);
    sd.bank = (sd.bank + 1) & mask;
    fwrite(&out_samp, sizeof(int32_t), n, out);
  }

  return 0;
}

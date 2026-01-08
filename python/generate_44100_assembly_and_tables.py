# Copyright 2025-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import sys
import filter_44100
errors = 0
the_filter = filter_44100.the_filter

def filter_value(i, filter_length, the_filter):
    if i < 0:
        return '    0'
    if i >= filter_length:
        return '    0'
    return '%d' % (the_filter[i])

filter_length = 2000
up_sample = 250
down_sample = 147

#for i in range(len(the_filter)):
#  the_filter[i] = i

for inputs in [5,10,20]:
  c_out = '#include <stdint.h>\n\n'
  max_outputs = inputs//5*8*17//16 + 1
  first = up_sample - 1
  samples_produced = 0

  struct_array = ''
  all_asms = set()
  all_filter_functions = set()
  for i in range(down_sample):
    actual_outputs = max_outputs - 1
    if first - actual_outputs * down_sample + inputs * up_sample >= up_sample:
        actual_outputs += 1
    io = 'i%d_o%d' % (inputs, actual_outputs)
    shifts = [0] * (actual_outputs)
    for z in range(actual_outputs):
        for k in range(actual_outputs):
            if k*250 + first-z*down_sample < 0:
                shifts[z] = k+1

    name = 'coefficients_x250_147_%s_n2000_%03d' % (io, i)
    c_out += 'int32_t %s[%s][8] = {\n' % (name, actual_outputs)
    for o in range(0, actual_outputs, 8):
      number_outputs = 8
      if actual_outputs - o < 8:
        number_outputs = actual_outputs - o
      out = [''] * number_outputs
      nout = 0
      for z in range(number_outputs):
          oo = ''
          for k in range(8):
              oo += filter_value((k+shifts[z+o])*up_sample + first,
                                     filter_length, the_filter) + ', '
          out[nout + number_outputs - 1 - z] = oo
          first -= down_sample
      nout += 8
      samples_produced += number_outputs
      c_out += '// outputs %d..%d\n' % (o, o+7)
      for j in range(number_outputs):
          c_out += '    { %s },\n' % (out[j])
    first += shifts[actual_outputs-1] * up_sample
    if first < 0:
      first += up_sample
    c_out += '};\n'
            
    shift_string = ''
    for j in shifts:
        shift_string += '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'[j]

    filter_function_name = 'filter_x250_147_%s_n2000_%s' % (io, shift_string)
    struct_array += '    {%s, &%s[0][0]},\n' %(filter_function_name,name)
    all_filter_functions.add(filter_function_name)
    c_out += '// Samples produced: %d\n\n' %(samples_produced)

    # This code can be optimised because it generates the same function over and over again
    # It is ok because it is saved in a set().

    asm = ''
    asm += '''#define FUNCTION_NAME %s

    .cc_top FUNCTION_NAME.function
    .type   FUNCTION_NAME,@function

    // Eats %d samples - produces %d

    .globl FUNCTION_NAME
    .globl FUNCTION_NAME.nstackwords
    .linkset FUNCTION_NAME.nstackwords, 0
    .linkset FUNCTION_NAME.maxcores, 0
    .linkset FUNCTION_NAME.maxtimers, 0
    .linkset FUNCTION_NAME.maxchanends, 0
    .add_to_set _fptrgroup.GROUP_NAME.group, FUNCTION_NAME, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.nstackwords.group, FUNCTION_NAME.nstackwords, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxcores.group, FUNCTION_NAME.maxcores, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxtimers.group, FUNCTION_NAME.maxtimers, FUNCTION_NAME
    .add_to_set _fptrgroup.GROUP_NAME.maxchanends.group, FUNCTION_NAME.maxchanends, FUNCTION_NAME
    .align 16
FUNCTION_NAME:
''' % (filter_function_name, inputs, actual_outputs)
    r11 = -1     # Not a useful value
    asm += '''
    { dualentsp 0               ; ldc r3, 32 }
    { vclrdr                    ; add r1, r1, 8 }
    { nop'''
    last_nop = len(asm)
    asm += '                     ; add r1, r1, 8 }\n'
    reversed_shifts = [0] * (len(shifts) + 1)
    for o in range(0, actual_outputs, 8):
        l = 8
        if o + l >= actual_outputs:
            l = actual_outputs - o
        for oo in range(l):
            reversed_shifts[o + l - 1 - oo] = shifts[o+oo]
    reversed_shifts[actual_outputs] = inputs

    vldc_necessary = [False] * (len(shifts)+1)
    last_index_loaded = -1
    for o in range(0, actual_outputs, 8):
        l = 8
        if o + l >= actual_outputs:
            l = actual_outputs - o
        for oo in range(l):
            if last_index_loaded != reversed_shifts[o+oo]:
                last_index_loaded = reversed_shifts[o+oo]
                vldc_necessary[o+oo] = True
    
    increments = [0] * len(shifts)
    for o in range(len(increments)):
        increments[o] = 4*(reversed_shifts[o+1] - reversed_shifts[o])
    
    first_zero_index = 0
    for o in range(len(increments)):
        if increments[o] != 0:
            if first_zero_index != o:
                increments[first_zero_index] = increments[o]
                increments[o] = 0
            first_zero_index = o + 1

    for o in range(0, actual_outputs, 8):
        l = 8
        if o + l >= actual_outputs:
            l = actual_outputs - o
        asm += '\n    // Elements %s increments %s\n' % (str(reversed_shifts[o:o+l]), str(increments[o:o+l]))
        for oo in range(l):
            the_inc = increments[oo+o]
            if vldc_necessary[oo+o]: #the_inc != 0 or oo == 0:
                if the_inc == -4:
                    asm += '    { vldc r1[0]                ; sub r1, r1, 4 }\n'
                elif the_inc == 0:
                    asm += '    { vldc r1[0]                ; nop'
                    last_nop = len(asm)
                    asm += '            }\n'
                elif the_inc == 32:
                    asm += '    { vldc r1[0]                ; add r1, r1, r3 }\n'
                elif the_inc == 4:
                    asm += '    { vldc r1[0]                ; add r1, r1, 4 }\n'
                elif the_inc == 8:
                    asm += '    { vldc r1[0]                ; add r1, r1, 8 }\n'
                elif the_inc == r11:
                    asm += '    { vldc r1[0]                ; add r1, r1, r11 }\n'
                elif the_inc > 0:
                    if last_nop is None:
                        asm += '    { ldc r11, %d }\n' %(the_inc)
                        asm += '    { vldc r1[0]                ; add r1, r1, r11 }\n'
                        r11 = the_inc
                    else:
                        asm += '    { vldc r1[0]                ; add r1, r1, r11 }\n'
                        asm = asm[:last_nop-3] + 'ldc r11, %d' % (the_inc) + asm[last_nop:]
                        last_nop = None
                        r11 = the_inc
                else:
                    errors += 1
                    print('ERROR unexpected increment at ', oo+o, increments)
            elif the_inc != 0:
                errors += 1
                print('Increment non zero when not loading ', oo+o, the_inc, increments, vldc_necessary)
            asm += '    { vlmaccr r2[0]             ; add r2, r2, r3 }\n'
        if o + 8 >= actual_outputs:
            asm += '    { vstr r0[0]                ; nop'
            last_nop = len(asm)
            asm += '            }\n'
        else:
            asm += '    { vstr r0[0]                ; add r0, r0, r3 }\n'
            asm += '    { vclrdr                    ; nop'
            last_nop = len(asm)
            asm += '            }\n'
    the_inc = inputs * 4
    if r11 != the_inc:
        if last_nop is None or the_inc > 63:
            asm += '    { ldc r11, %d }\n' %(inputs * 4)
        else:
            asm = asm[:last_nop-3] + 'ldc r11, %d' % (the_inc) + asm[last_nop:]
            last_nop = None
        r11 = the_inc
    asm += '''    { vldd  r1[0]               ; sub r1, r1, r11 }
    { vstd  r1[0]               ; nop }
    { retsp 0                   ; ldc r0, %d }
    .cc_bottom FUNCTION_NAME.function

#undef FUNCTION_NAME
  
''' % (actual_outputs)
    all_asms.add(asm)

  do_not_edit = '// DO NOT EDIT, generated by lib_sigma_delta/host/generate_44100_assembly_and_tables.py lib_sigma_delta_git_directory\n\n'

  with open(sys.argv[1] + '/lib_sigma_delta/src/filter_x250_147_%s_n2000.S' % (io), 'w') as fd:
      print(do_not_edit, file=fd)
      print('#define GROUP_NAME filter_x250_147', file=fd)
      print('.weak       _fptrgroup.GROUP_NAME.group', file=fd)
      print('.weak       _fptrgroup.GROUP_NAME.nstackwords.group', file=fd)
      print('.weak       _fptrgroup.GROUP_NAME.maxcores.group', file=fd)
      print('.weak       _fptrgroup.GROUP_NAME.maxtimers.group', file=fd)
      print('.weak       _fptrgroup.GROUP_NAME.maxchanends.group', file=fd)

      print('    .issue_mode dual', file=fd)
      for i in sorted(all_asms):
          print(i, file=fd)
          
  with open(sys.argv[1] + '/lib_sigma_delta/src/filter_x250_147_%s_n2000.h' % (io), 'w') as fd:
      print(do_not_edit, file=fd)
      print(c_out, file=fd)
      for i in sorted(all_filter_functions):
          print('extern int %s(int32_t *output, int32_t *input, int32_t *coefficients);' % (i), file=fd)

      print('\nstruct filter_x250_147_phases filter_x250_147_i%d_phases[%d] = {' % (inputs, down_sample), file=fd)
      print(struct_array, file=fd)
      print('};', file=fd)

if errors != 0:
    print('FAIL')

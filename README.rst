:orphan:

########################
lib_sw_dac: Software DAC
########################

:vendor: XMOS
:version: 0.1.0
:scope: Demo
:description: Software DAC for xcore.ai
:category: Audio
:keywords: Audio, DAC, PWM
:devices: xcore.ai

*******
Summary
*******

Software DAC for xcore comprising of upsampling, sigma-delta modulator and PWM generation.

********
Features
********

* Supports 48000, 96000 and 192000 Hz sample rate, stereo output
* Uses two hardware threads, two one-bit ports, one timer, one clock-block and 32 kB of RAM

************
Known issues
************

* Fixed configuration only currently (SF = Standard Fidelity)
* 192 kHz input requires 85 MHz thread speed (max 7 threads at 600 MHz core clock). 96 kHz and 48 kHz can support 8 used threads.

****************
Development repo
****************

* `lib_sw_dac <https://www.github.com/xmos/lib_sw_dac>`_

**************
Required tools
**************

* XMOS XTC Tools: 15.3.1

*********************************
Required libraries (dependencies)
*********************************

* None

*************************
Related application notes
*************************

The following application notes use this library:

* AN02020: Adding a software DAC to USB Audio

Please also see the /examples directory for simpler usage examples.

*******
Support
*******

This package is supported by XMOS Ltd. Issues can be raised against the software at: http://www.xmos.com/support



:orphan:

########################
lib_sw_dac: Software DAC
########################

:vendor: XMOS
:version: 1.0.0
:scope: General Use
:description: Software DAC for xcore.ai
:category: Audio
:keywords: DAC
:devices: xcore.ai

*******
Summary
*******

Software DAC for ``xcore.ai`` comprising of upsampling, sigma-delta modulator and PWM generation.

********
Features
********

* Supports 44100, 48000, 88200, 96000 and 176400 & 192000 Hz sample rate, stereo output
* Uses two hardware threads, two one-bit ports, one timer, one clock-block and 32 kB of RAM

************
Known issues
************

* Fixed configuration only currently (SF = Standard Fidelity)
<<<<<<< HEAD
=======
* The API requires that the software DAC is continually fed samples at the input
  sample rate. Failure to do so will result in full scale DC output. Please
  ensure your application disables the hardware output stage before stopping
  feeding samples to the software DAC.
>>>>>>> develop

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



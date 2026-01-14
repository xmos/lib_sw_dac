lib_sw_dac change log
=====================

1.0.0
-----

  * First public release
  * FIXED: PWM outputs DC level zero when not fed with samples
  * FIXED: PWM left and right outputs remain synchronised
  * FIXED: Sending 0 sample rate now quits Software DAC

0.2.0
-----

  * ADDED: Support for 44.1, 88.2 and 176.4 kHz sample rates
  * RESOLVED: Timing test now uses true 24 MHz clock
  * RESOLVED: Poor SNR in right channel at 96 kHz issue #29
  * RESOLVED: Initial PWM output frame now 50% duty

0.1.0
-----

  * Inital engineering version


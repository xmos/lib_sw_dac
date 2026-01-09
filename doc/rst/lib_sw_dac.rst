########################
lib_sw_dac: Software DAC
########################

|newpage|


************
Introduction
************

The Software DAC is a DSP driven component which takes a PCM input signal and 
converts it to a 1-bit output, at a high rate, which is then filtered using
simple filtering to produce an analogue signal. This removes the need for
traditional DAC components in many systems.

The software defined DAC comprises four major parts:

* An *upsampler* that consumes a 16-, 24-, or 32-bit signal at the *audio
  sampling rate* (eg, 48 kHz), and produces a 32-bit signal at the *PWM
  pulse rate* (eg, 1.5 MHz)

* A *modulator* that takes a 32-bit signal at the *PWM pulse rate* and
  computes *discrete* PWM values at that rate. Eg, it may use eight
  discrete levels [-3.5, -2.5, -1.5, -0.5, +0.5, +1.5, +2.5, +3.5] and
  approximate the signal 0.0 by outputting -0.5, +0.5, -0.5, +0.5, -0.5,
  +0.5, ...

* A PWM generator that creates a PWM signal at the *PWM pulse rate* and
  outputs to GPIO pins on the *xcore.ai* device. The
  edges of the PWM signal are precisely timed at discrete points in time.
  In order to achieve this, a *master clock* is used that is a whole
  multiple of the *pwm pulse rate*.

* An external analogue hardware and low-pass filter and amplifier that filter out inaudible signals and drive the analogue signal to a headphone or speaker with a suitable voltage and impedance.


These four parts are shown in :numref:`sw_dac_stages`.

.. _sw_dac_stages:

.. uml::
   :width: 100%
   :caption: Software DAC Stages

    @startuml
    
    skinparam rectangle {
    BackgroundColor #cceeff
    BorderColor #4477aa
    RoundCorner 5
    }

    rectangle "Upsampler " as S1
    rectangle "Sigma Delta Modulator" as S2
    rectangle "PWM Generator" as S3
    rectangle "Analog Filter" as S4

    :PCM samples: --> S1
    S1 -right-> S2
    S2 -right-> S3
    S3 -right-> S4
    S4 --> :Analogue Output:

    @enduml

Each of these stages can be made as refined as one likes. The quality of
the output signal is typically determined by the weakest of these
stages. Hence, there is no point in making one of the stages a lot
better than all the others and instead should be balanced in terms of performance.

Example configurations are provided.

|newpage|

*********
Operation
*********


The signal is processed in fixed-point arithmetic, but for the purpose
of explaining the operation, we assume that a full-scale signal is represented by real numbers in
the range [-1.0..1.0]. The signal's internal representation takes many
forms (Q3.29, Q4.28), but these are hidden from the user as long as all
filter and modulator coefficients are represented in Q2.30 format.

DC removal
----------

The digital pipeline starts by removing the DC component in the signal
using a high-pass filter with a cut-off frequency of around 1 Hz.
Performing this task digitally can reduce BOM cost by removing the
need for AC coupled stages. DC removal can be switched off if it is undesirable.
For details, see :c:macro:`SW_DAC_DC_REMOVAL_TIME_CONSTANT`.

Up-sampling
-----------

Assuming an input sample rate of 48 kHz and an PWM pulse rate of 1.5 MHz
the audio signal needs to be upsampled by about 32x. The filter(s) used in
upsampling can be made longer (less ripple), or shorter (less compute) and
can be made symmetric (constant group delay) or asymmetric (less
latency) as needed. The number of filters can be changed too, although for pragmatic
reasons it is good to start with 2x upsamplers as that creates natural
input points for 96 and 192 kHz sample rates if multiple input sample rate is required.

In order to support the 44,100 family, one can either support:

* A dual master clock configuration.
* A fractional sample rate converter at the input stage.
* A design the final filters to perform the fractional conversion at that stage.

PWM pulse rate and levels
-------------------------

The PWM pulse rate governs the frequency of PWM pulses. A high pulse rate
produces a better noise floor but requires more compute. A lower pulse rate
may be preferable when driving a class-D style amplifier which may have an
upper limit on output stage switching frequency.

Each PWM pulse has a discrete length. The number of lengths available is
called the number of PWM levels. Higher numbers of PWM levels produce a
lower noise floor. The PWM levels used are always symmetrical around zero,
enabling negative and positive signals of the same magnitude to be created.

Hence, if the number of PWM levels is odd, then the middle level will be 0,
and the values -1, +1, -2, +2 etc will be used. If the number of PWM levels
is even, then the two middle values will be +/-0.5, and the values -1.5,
+1.5, -2.5, +2.5 etc will be used.

.. note::
    At power on, the ``xcore.ai`` GPIO ports are set to high impedance with a
    weak pull-down enabled. This will define the initial state of the output 
    ports until the software DAC is configured and operating.
    The hardware **must** be designed so that power output stage is disabled
    during this state to avoid damaging the load.


Master clock frequency
----------------------

The master clock frequency is an integer multiple of the PWM pulse rate,
because each PWM pulse is constructed of a whole number of master clock
periods. The master clock is either the number of PWM levels multiplied by the PWM
pulse rate, in which case PWM pulses will be asymmetric, or, PWM pulses can
be are made symmetrical requiring the master clock to be twice that rate.

Modulator design
----------------

The modulator comprises the following parameters:

* A matrix that governs the modulator design. The modulator has a state,
  which is the value of each of the accumulators: a vector of six values
  for a sixth order modulator. Given the current state of the modulator,
  the new input value, and the quantized output value, the matrix is used
  to compute the next state. Hence, on each iteration the following
  computation takes place::

     vector[0] = input
     vector[1] = quantize(state[5])
     vector[2:7] = state
     state = matmul(CM, vector)

  where state is a vector with six elements, enabling a modulator of up to
  sixth order to be implemented.

  All elements of the matrix must have a magnitude less than 2.0. Note that
  there is no post-multiplication: if your design has a multiplication
  between the final accumulator and the quantizer, then you should arrange
  the matrix so that that multiplier is 1.0. This demand can be achieved by
  dividing that column of the matrix by the final multiplier, and
  multiplying the associated row by the final multiplier. The same method
  can be used to make sure that all elements have a magnitude of less than
  2.0. Note that it is undesirable for the first column of the matrix
  (which governs the inputs) to have very small values as that prohibits
  low magnitude signals (say, less than -130 dB) to be modulated.

* A scale factor that converts the full scale input value (a number in the
  range -1.0..1.0) to an input value for the modulator. The number is
  specific to the modulation matrix and is limited by the number of PWM
  levels. Say that the maximum PWM levels are +/- 3.5, then the scale
  factor will be less than 3.5. If the scale is too close to full scale PWM
  you will get distortion at full-scale, so a typical scale value may be
  3.0 or 2.9.

* A limiter for the full-scale input value. You can set the limiter to 1.0,
  or you may choose to set it higher, depending on whether you would want
  to faithfully represent an upscaled sine-wave of a quarter of the input
  sample rate. The limiter must be less than 2.0 and greater or equal to
  1.0.

Signal negation
---------------

Depending on the final output circuitry, it may be desirable to invert the 
output signal. An optional negate parameter is available to do this,
see :c:macro:`SW_DAC_NEGATE`.

Pre-distortion
--------------

Before the signal is passed into the modulator it can be pre-distorted.
Pre-distortion includes 2 types of signal compensation:

* Flat compensation
* PWM compensation

Flat compensation applies a fixed transformation to the input signal 
to counteract non-linearities introduced by the analogue output stage.
This adjustment is hardware-dependent and helps reduce distortion across
the entire signal range.

PWM compensation modifies the input signal based on the characteristics of the
modulation matrix and PWM process, typically tailored to correct specific artifacts
introduced by the digital modulator, improving overall signal fidelity.

Both are implemented in the form of ``c2 * x^2 + c3 * x^3``,
with a high-pass filter for the PWM compensation.
If the user wishes to disable pre-distortion, they would need to set all coefficients to zero.

Generation of master clock and synchronisation
----------------------------------------------

The master clock may be generated outside the XCORE (using a crystal
oscillator or external PLL), or inside the XCORE (using the secondary PLL
in the XCORE or using the core PLL).

An externally generated PLL can support lower jitter than an internally generated
PLL creating a better noise floor.

When using an externally-generated clock you must resynchronise the PWM
stream with that clock using a D-type flip-flop in order to minimise jitter
in the PWM edges since the GPIO output from the XCORE is synchronised to the
internal core clock which typically operates at 600 MHz or 800 MHz.

Analog filter and amplifier
---------------------------

Assuming that the direct signal is too low an amplitude level or too high impedance, you can
either:

* Use an external head-phone amplifier.

* Directly drive a class-D style amplifier (H-bridge)

It is recommended to use a low-pass filter if driving a head-phone amplifier to filter high frequency signal components.
Depending on the required output bandwidth, a second order, linear-phase low pass filter with a
cut-off point between 24 kHz and 48 kHz may be suitable.

.. caution::
    The audio output is driven using a PWM on a GPIO pin. A DC level of 0 is represented by a 50% duty cycle output.
    This means that, at any point, if the power output stage is enabled and the software DAC
    is not operating, the output will be driven with full scale DC which may cause damage to the load (headphones, speakers, etc).

    For a practical design, it means that the sequencing for powering up and down of the power stage
    must be carefully designed so that full scale DC is never driven. The application note ``AN02020``
    (Adding a software DAC to USB Audio) provides an example of this.

    It is also highly recommended to enable the watchdog timer, which will trigger a hard reset to the
    chip if it is not continually kicked. This will force the chip GPIOs to enter their default state (not driven with weak pull down)
    and the hardware power output stage should be designed so that it is disabled when the GPIOs are in this state.
    An API for the watchdog timer is provided in this library, see :ref:`watchdog_api<watchdog_api>`.

    When developing, a software exception or even stopping the code using the debugger will also cause a full scale output to be driven.
    During the development phase of a project using this library, the use of DC blocking capacitors and/or a current limited power supply is recommended.


|newpage|

****************************
Customising The Software DAC
****************************

Starting point for the design
-----------------------------

As the design is highly configurable we provide a set of options to start
with. They have been chosen so that you can make the design better (and
more expensive) or cheaper (with a higher noise floor).

.. note:: All the design parameters are tied together by the choice of 
    modulator. The modulator is designed for a specific number of PWM levels at
    a specific pulse rate.

Modulator sd_coeffs_o6_f1_5_n8
------------------------------

This is a 6th order modulator that outputs eight PWM levels at a 1.5 MHz
pulse rate. The eight levels are [-3.5, -2.5, -1.5, -0.5, +0.5, +1.5, +2.5,
+3.5]. The default scale is 2.9 and the default limit is 2.9. The modulator
introduces a delay of 6/1.5 MHz = 4 us.

The default pre-distortion values are tuned to remove second and third harmonics
from this modulator:

* flat_x2: 1.0/120000

* flat_x3: -1.0/250000

* pwm_x2: 3.0/157

* pwm_x3: 0.63/157

* scale: 2.8544

* limit: 2.8684735298

Upsampler filter_x125_4
-----------------------

The default upsampler is

* 48 kHz in, x2, 96 kHz out, 80 taps, 416 us latency

* 96 kHz in, x2, 192 kHz out, 32 taps, 83 us latency

* 192 kHz in, x2, 384 kHz out, 16 taps, 20.8 us latency 

* 384 kHz in, x2, 786 kHz out, 16 taps, 10.4 us latency

* 786 kHz in, x125/64, 1500 kHz out, 1000 taps, 10.4 us latency

With all filters symmetrical and a constant group delay.

If desirable, one could change the final filter to 2x, the clock to 24.576,
and the PWM pulse rate to 1.536 MHz without many other changes.

Master clock and analogue amplifier
-----------------------------------

By default, we create symmetrical PWM pulses, hence we use a master clock
of 1.5 MHz x 8 x 2 = 24 MHz. Our example design uses an external 24 MHz
oscillator with an external D-type resynchroniser that use a low-noise LDO
not used by the digital circuitry. For cost reduction, one can generate 24 MHz
directly from a shared oscillator with the XCORE, or directly from the
XCORE if ran at 600 MHz (600/25 = 24 MHz); this will raise the noise floor
as this clock has more jitter.

The example design uses a low-noise LDO with a D-type flop to resynchronise
the signal, leading into a low-cost head-phone amplifier using low-noise
resistors. 


|newpage|

**************************
Performance Considerations
**************************

The default headphone amplifier configuration produces audio performance 
characterised by :numref:`default_performance` which is for sw_dac_sf()
where `sf` means standard fidelity:

.. table:: Typical performance
   :name: default_performance

   ================= =============== ===============
                     Analog output   Digital output
   ================= =============== ===============
   Dynamic Range      106.5 dB        122 dB
   THD                -117 dB         -122 dB
   THD+N              -120 dB         -117 dB
   ================= =============== ===============

.. note::
    In this table, the digital output is the signal that the XCORE produces, assuming an
    ideal output stage, post D-type resynchronisation flipflop. The Analog output is the signal as measured on the
    3.5mm headphone jack. The difference between the two is due to
    the design of the analog stage where noise is added in from various
    sources.

Three sorts of inaccuracies are created in the process of Digital to Analogue
conversion:

Noise
-----

The noise floor comprises the (root-mean-squared) sum of all noise sources.
These noise sources may include but are not limited to:

* Noise floor of the modulator, you can reduce this by running the
  modulator faster and/or with a higher order

* Noise of the analogue power supplies. You can reduce these by using
  better power supplies.

* Jitter in the clock. You can reduce this by running the clock faster, and
  by using a low-jitter clock source.

* Noise of analog components (resistors, capacitors, op-amps), you can
  reduce these by using more expensive components.

Harmonics (of the input signal)
-------------------------------

The harmonics of the signal are mostly introduced by the modulator, and to
a lesser extent, by the analogue amplifier. They can be reduced by pre-distorting the signal.

Cross-talk (between channels)
-----------------------------

Cross-talk is introduced because of shared analogue components between
multiple channels (power-supplies, resynchronisers, etc). Cross-talk can be
reduced by replicating those components.

The default design may be sufficient for many applications, but for some
applications you may choose to spend more BOM and XCORE MIPS on a solution with
less noise, harmonics, and/or cross-talk.


|newpage|

.. _api_section:

*************
API Reference
*************

The Software DAC main task, ``sw_dac_sf()``, comprises of two threads which can be seen, alongside the sample producing app, in :numref:`software_dac_usage`


.. _software_dac_usage:

.. uml::
   :width: 100%
   :caption: Software DAC usage task diagram

   @startuml
   left to right direction
   circle producer_app

   rectangle "Software DAC"{
       circle upsampling_filter
       circle sigma_delta_task
   }

   rectangle output_ports

   producer_app --> upsampling_filter : API channel
   upsampling_filter --> sigma_delta_task : internal channel
   sigma_delta_task --> output_ports
   @enduml


All functions can be accessed via the ``sw_dac.h`` header::

  #include "sw_dac.h"

You will also have to add ``lib_sw_dac`` to the application's ``APP_DEPENDENT_MODULES`` list in
`CMakeLists.txt`, for example::

    set(APP_DEPENDENT_MODULES "lib_sw_dac")


Configuration Header
--------------------

Various settings are set statically. These are picked up from your project directory from the
``sw_dac_conf.h`` file. Default settings are used if this file is not present along with 
a warning that the defaults are being used. You may create an empty ``sw_dac_conf.h`` if
you wish to use default settings and remove the compiler warning.

.. doxygendefine:: SW_DAC_NEGATE

.. doxygendefine:: SW_DAC_DC_REMOVAL_TIME_CONSTANT

The full listing of ``sw_dac_conf_default.h`` is shown below.

.. literalinclude:: ../../lib_sw_dac/api/sw_dac_conf_default.h
    :language: c
    :lines: 12-27

|newpage|


Supporting types
----------------

The following type is used to configure the software DAC components.

.. doxygenstruct:: sw_dac_sf_t

It is not necessary to directly modify this structure and can be considered a black-box.
Please configure it using the :ref:`sw_dac_sf_init()<init_api_sf>` function.


Creating a software DAC instance
--------------------------------

.. _init_api_sf:

.. doxygenfunction:: sw_dac_sf_init

.. doxygenfunction:: sw_dac_sf

For the initialisation of the structure using ``sw_dac_sf_init()`` one can choose from the following
pre-defined modulators:

.. doxygenvariable:: sd_coeffs_o6_f1_5_n8


|newpage|


Supporting functions
--------------------

.. _watchdog_api:

.. doxygendefine:: PLATFORM_OSCILLATOR_FREQUENCY_HZ

.. doxygenfunction:: watchdog_init

.. doxygenfunction:: watchdog_reset_counter

.. doxygenfunction:: watchdog_has_tiggered

|newpage|

.. _example_design_section:

*******************
Example application
*******************

A simple example is provided that generates a 62.5 Hz sine wave and outputs it to both channels of the software DAC as sigma-delta modulated PWM. 
The hardware used is the `xcore.ai development board <https://www.xmos.com/xk-evk-xu316>`_ although any ``xcore.ai`` development board should work 
as long as two one bit ports are available and can be monitored using an oscilloscope or logic analyser.

.. warning::
    The used hardware platform *does not* include the necessary D-type flip-flops or analogue output stage and so the performance will be significantly worse than that described in :numref:`default_performance`.
    However, using an oscilloscope will still allow you to see the PWM output and verify correct operation. It is also possible to connect a pair of headphones directly to the output ports, but this is not
    recommended without a DC blocking capacitor as it may damage the headphones or the hardware. Further, the output will be quite quiet due to the limited output
    drive capability of the GPIO pins and lack of a gain stage.


Building the example
--------------------

This section assumes that the `XMOS XTC Tools <https://www.xmos.com/software-tools/>`_ have been
downloaded and installed. The required version is specified in the accompanying ``README``.

Installation instructions can be found `here <https://xmos.com/xtc-install-guide>`_.

Special attention should be paid to the section on
`Installation of Required Third-Party Tools <https://www.xmos.com/documentation/XM-014363-PC/html/installation/install-configure/install-tools/install_prerequisites.html>`_.

The application is built using the `xcommon-cmake <https://www.xmos.com/file/xcommon-cmake-documentation/?version=latest>`_
build system, which is provided with the XTC tools and is based on `CMake <https://cmake.org/>`_.

The ``lib_sw_dac`` software ZIP package should be downloaded and extracted to a chosen working
directory.

To configure the build, the following commands should be run from an XTC command prompt::

    cd lib_sw_dac/examples/app_sf_sine
    cmake -G "Unix Makefiles" -B build

If any dependencies are missing they will be retrieved automatically during this step.

The application binaries should then be built using ``xmake``::

    xmake -j -C build

Binary artifacts (.xe files) will be generated under the appropriate subdirectories of the
``app_sf_sine/bin`` directory — one for each supported build configuration.

For subsequent builds, the ``cmake`` step may be omitted.
If ``CMakeLists.txt`` or other build files are modified, ``cmake`` will be re-run automatically
by ``xmake`` as needed.


Running the example
-------------------

From an XTC command prompt, the following command should be run from the ``examples/app_sf_sine``
directory::

    xrun ./bin/app_sf_sine.xe

Alternatively, the application can be programmed into flash memory for standalone execution::

    xflash ./bin/app_sf_sine.xe


The PWM output may be monitored on pins ``36`` and ``38`` of connector J10 of the `xcore.ai development board <https://www.xmos.com/xk-evk-xu316>`_
which correspond to ports ``XS1_PORT_1M`` and ``XS1_PORT_1O`` respectively.

An oscilloscope capture of the output can be seen in :numref:`pwm_example_output`. Here you can clearly see the PWM output switching at 1.5 MHz
and the discrete PWM levels as the sine wave varies slowly over time. The top trace is the left output, on which the waveform is triggered,
and the bottom trace is the right output.

.. _pwm_example_output:
.. figure:: ../images/pwm_example.png
   :alt: PWM Example Output
   :width: 100%
   :align: center

   PWM Example Output


*************************************
Standard Fidelity Software DAC Design
*************************************

The standard fidelity software DAC aims to output good quality audio on
readily available and simple hardware. To this end, the system has been
designed using the following parameters:

* A single master clock of 24 MHz is assumed. This allows one to share the
  master clock with the clock for the XCORE device, potentially reducing
  the BOM.

* 1.5 MHz sample rate for an eight level PWM.
  
* Support for 44100, 48000, 88200, 96000, 176400, 192000 Hz sample rates.

* Pre-distort in order to reduce the effect of second and third harmonics

This produces a logical UPWM signal with a dynamic range of 122 dB. It
supports a variety of output stages that can achieve up to 122 dB dynamic
range.

Alternative designs could use:

* Multiple master clocks for different frequency families. This adds BOM
  but simplifies filtering and has the potential to slightly increase the
  SNR

* Master clocks that are a whole multiple of the input frequencies, eg,
  24.576 MHz rather than 24.000 MHz. This also simplifies filtering.

* A significantly higher master clock, for example 48 MHz. This reduces the
  noise floor and simplifies the hardware filter as the pulse rate can be
  doubled to 3 MHz.

* A significantly lower master clock, enabling FETs to be driven directly.
  For example, 600 kHz.

* Increase the number of taps in the filters, enabling finer control over
  the ripple in the signal and drop off in the 20-22 kHz band.

* Use minimum phase filters in order to reduce the latency of the DAC. This
  is simply a matter of changing the filter coefficients.

* Use longer filters with improved roll-off at the cost of extra MIPS.

Filters
-------

The 48 kHz family is upsampled as follows:

.. _48000_family_filters:
.. figure:: ../images/48000_family_filters.*
   :height: 175mm

   Individual filter responses. From top to bottom 48 to 96 kHz; 96 to 192 kHz; 192
   to 384 kHz; 384 to 768 kHz; 768 kHz to 1.5 MHz.

* An input signal of 48,000 Hz is upsampled by 2x using a filter with 80
  taps. This produces a signal of 96,000 Hz.
  
* An input signal of 96,000 Hz (or an upsampled signal of 48,000 Hz) is
  upsampled by 2x using a filter with 32 taps. This produces a signal of
  192,000 Hz.

* An input signal of 192,000 Hz (or an upsampled signal of 48,000/96,000
  Hz) is upsampled by 2x using a filter with 16 taps. This produces a
  signal of 384,000 Hz.

* Any input signal that has been upsampled to 384,000 Hz is upsampled by 2x
  using a filter with 16 taps. This produces a signal of 768,000 Hz.

* Any input signal that has been upsampled to 768,000 Hz is upsampled by a
  125x and downsampled by 64x using a filter with 1,000 taps. This produces
  a signal of 1,500,000 Hz.

.. _48000_family_responses:
.. figure:: ../images/48000_family_responses.*
   :width: 90%

   Combined filter response for 48, 96, and 192 kHz sample rates.

The individual filter responses are shonw in :numref:`48000_family_filters`; the combined
filter response is shown in :numref:`48000_family_responses`. 
Combined, these filters have an impulse response as shown in
:numref:`48000_family_impulses`. These images show the output
of an impulse at time 0. The highlighted value in the middle indicates the
peak of the impulse. The X axis is labelled in input samples.

.. _48000_family_impulses:
.. figure:: ../images/48000_family_impulses.*
   :width: 90%

   Impulse response for an impulse at 48, 96, and 192 kHz sample rate. Each
   figure shows the delay that the filters introduce as measured in samples
   at the input sample rate.
   
The 44.1 kHz family is upsampled as follows:

* An input signal of 44,100 Hz is upsampled by 2x using a filter with 80
  taps. This produces a signal of 88,200 Hz.

* An input signal of 88,200 Hz (or an upsampled signal of 44,100 Hz) is
  upsampled by 2x using a filter with 32 taps. This produces a signal of
  176,400 Hz.

* An input signal of 176,400 Hz (or an upsampled signal of 44,100/88,200
  Hz) is upsampled by 2x using a filter with 16 taps. This produces a
  signal of 344,800 Hz.

* Any input signal that has been upsampled to 344,800 Hz is upsampled by 5x
  and downsampled by 2x using a filter with 40 taps. This produces a signal
  of 862,000 Hz.

* Any input signal that has been upsampled to 862,000 Hz is upsampled by a
  250x and downsampled by 147 using a filter with 2,000 taps. This produces
  a signal of 1,500,000 Hz.

Hence, any input signal is upsampled to 1.5 MHz after which a modulator
picks one of eight UPWM levels which are output on a 24 MHz clock,
producing a 1.5 MHz pulse rate.

The 2x filters are all brick-wall filters with pass band in the audible
band, and a stop band in the top half of the frequency bands. the filtes
that are used prior to an up and down filters are designed to primarily
null out the first, second, third, and fourth mirror images of the audio
band; with a pass band in the audio band.

Gibbs ringing
-------------

This sequence of upsamplers will, like any upsampler, cause ringing on an
step change. For example, a signal [-1, -1, -1, -1, -1, +1, +1, +1,
+1, +1] will ring before and after the step change. Similarly, a signal
[-1, -1, +1, +1, -1, -1] will convert to a sinewave with an amplitude of
sqrt(2); well above full scale.

The standard fidelity configuration of the software dac is programmed to
clip any ringing more than 1% above full scale; this to avoid instability
in the modulator, and to give us maximum dynamic range for normal audio
signals.

A different configuration could change this trade-off. For example, the
clipping can be set higher enabling the ringing to not be clipped at the
expense of some dynamic range.

Modulator
---------

The modulator used is a 6th order modulator that outputs to eight levels
[-3.5, -2.5, -1.5, -0.5, +0.5, +1.5, +2.5, +3.5]. The modulator can accept
signals up to 0.81x full scale, which equals numbers in the range
+/-0.81x3.5 - [-2.835..2.835]. If numbers higher than that are presented
the modulator may go unstable: it will need to use +4.5 or -4.5 which is
not part of its output levels.

UPWM
----

The UPWM output is symmetric and outputs the following patterns:

====== ====================
Level  Pattern
====== ====================
-3.5   ``0000000010000000``
-2.5   ``0000000111000000``
-1.5   ``0000001111100000``
-0.5   ``0000011111110000``
+0.5   ``0000111111111000``
+1.5   ``0001111111111100``
+2.5   ``0011111111111110``
+3.5   ``0111111111111111``
====== ====================

This means that we achieve a maximum level of 93.75% and a minimum level of
6.25%; at -1.16 dB of full scale voltage swing.

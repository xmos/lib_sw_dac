// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef __watchdog_timer__
#define __watchdog_timer__

/**
 * The crystal input speed to the xcore. Used for configuring the watchdog timer
 * timeout period. */
#ifndef PLATFORM_OSCILLATOR_FREQUENCY_HZ
#define PLATFORM_OSCILLATOR_FREQUENCY_HZ        24000000
#endif

/**
 * Function that initialises the watchdog timer. Since power stages are being
 * driven directly by xcore ports, it is recommended to use the on-chip hardware
 * watchdog timer to guard in case of any software crashes which will result
 * in full scale DC being driven into the load.
 * 
 * The hardware should be configured such that, in the case of a chip reset,
 * the output stages are not energised. By default after POR, xcore ports are
 * high impedance with a weak pull-down resistor.
 * 
 * Note that, after initialisation, the watchdog is already enabled and counting. Please
 * call watchdog_reset_counter() periodically following initialisation.
 * 
 * \param   microseconds    The initial number of microseconds before the watchdog timer
 *                          expires and resets the chip. Typically you can set
 *                          this to be greater than the time required to get to the first
 * 							call to watchdog_reset_counter(). This is a 16 bit field meaning
 * 							that the maximum number of microseconds is 65535, or 65 milliseconds.
 *  
 */
void watchdog_init(unsigned microseconds);

/**
 * Function that resets the watchdog timer. Call periodically to avoid the chip
 * resetting.
 * 
 * \param   microseconds    The number of microseconds before the watchdog timer
 *                          expires and resets the chip. Typically you can set
 *                          this to twice the sample period of the producer task.
 * 							This is a 16 bit field meaning that the maximum number
 * 							of microseconds is 65535, or 65 milliseconds.
 *  
 */
void watchdog_reset_counter(unsigned microseconds);

/**
 * Function that returns a boolean as to whether the chip has been reset by 
 * a watchdog timer expiry event. Can be called at startup to check for a WDT
 * expiry event.
 * 
 * \returns    1 = WDT has expired causing the chip to reset. 0 = chip was reset normally.
 *  
 */
int watchdog_has_tiggered(void);

#endif // __watchdog_timer__
#ifndef MIDI1_CLOCK_TIMER
#define MIDI1_CLOCK_TIMER
/**
 * @file @file midi1_clock_timer.h 
 * @brief MIDI1.0 clock for zephyr RTOS using _software_ timer.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251214
 */
#include <zephyr/kernel.h>	/* k_timer */
#include <zephyr/sys/atomic.h>	/* atomic_t, atomic_get/set */
#include <stdint.h>		/* uint32_t, uint16_t */
#include <stddef.h>		/* NULL */

/**
 * @brief Initialize MIDI clock subsystem with your MIDI device handle.
 * @param midi1_dev pointer to usb midi device
 *
 * @note Call once at startup before starting the clock.
 */
void midi1_clock_init(const struct device *midi1_dev);

/**
 * @brief Start periodic MIDI clock. interval_us must be > 0.
 *
 * @note Uses k_timer_start with
 * @note the same period for initial and repeat time.
 * @param interval_us interval in us
 */
void midi1_clock_start(uint32_t interval_us);

/**
 * @brief  Stop the clock
 */
void midi1_clock_stop(void);

/**
 * @brief start the clock
 * @param sbpm scaled BPM insteaf of 123.23 it is: 12323
 */
void midi1_clock_start_sbpm(uint16_t sbpm);

#endif				/* MIDI1_CLOCK_TIMER */
/* EOF */

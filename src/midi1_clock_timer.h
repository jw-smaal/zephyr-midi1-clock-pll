#ifndef MIDI1_CLOCK_TIMER
#define MIDI1_CLOCK_TIMER
/*
 * MIDI1.0 clock for zephyr RTOS using software timer. 
 *
 * By Jan-Willem Smaal <usenet@gispen.org> 20251214
 */
#include <zephyr/kernel.h>	/* k_timer */
#include <zephyr/sys/atomic.h>	/* atomic_t, atomic_get/set */
#include <stdint.h>		/* uint32_t, uint16_t */
#include <stddef.h>		/* NULL */

/*
 * Initialize MIDI clock subsystem with your MIDI device handle. Call once at
 * startup before starting the clock.
 */
void midi1_clock_init(const struct device *midi1_dev);

/*
 * Start periodic MIDI clock. interval_us must be > 0. Uses k_timer_start with
 * the same period for initial and repeat time.
 */
void midi1_clock_start(uint32_t interval_us);

/* Stop the clock */
void midi1_clock_stop(void);

void midi1_clock_start_sbpm(uint16_t sbpm);

#endif				/* MIDI1_CLOCK_TIMER */
/* EOF */

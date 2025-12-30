#ifndef MIDI1_CLOCK_COUNTER
#define MIDI1_CLOCK_COUNTER
/**
 * @brief MIDI1.0 clock for zephyr RTOS using software timer.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251214
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>	/* k_timer */
#include <zephyr/sys/atomic.h>	/* atomic_t, atomic_get/set */
#include <stdint.h>		/* uint32_t, uint16_t */
#include <stddef.h>		/* NULL */

#ifndef COUNTER_DEVICE
#define COUNTER_DEVICE pit0_channel0
#endif

/*
 * Initialize MIDI clock subsystem with your MIDI device handle. Call once at
 * startup before starting the clock.
 */
void midi1_clock_cntr_init(const struct device *midi1_dev);

uint32_t midi1_clock_cntr_cpu_frequency(void);

/*
 * Start periodic MIDI clock. interval_us must be > 0. 
 */
void midi1_clock_cntr_start(uint32_t interval_us);

/* Start clock with MIDI ticks as argument (more accurate) */ 
void midi1_clock_cntr_ticks_start(uint32_t ticks);

void midi1_clock_cntr_update_ticks(uint32_t new_ticks);

/* Stop the clock */
void midi1_clock_cntr_stop(void);


/**
 * @brief Generate MIDI1.0 clock
 */
void midi1_clock_cntr_gen(const struct device *midi, uint16_t sbpm);

void midi1_clock_cntr_gen_sbpm(uint16_t sbpm);

#endif				/* MIDI1_CLOCK_TIMER */
/* EOF */

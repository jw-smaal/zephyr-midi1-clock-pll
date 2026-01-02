#ifndef MIDI1_CLOCK_COUNTER
#define MIDI1_CLOCK_COUNTER
/**
 * @file midi1_clock_counter.h
 * @brief MIDI1.0 clock for zephyr RTOS using hardware clock timer.
 * pit0_channel0
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

/**
 * @brief Initialize MIDI clock subsystem with the MIDI device handle.
 *
 * @note Call once at startup before starting the clock.
 * @param midi1_dev MIDI device pointer
 */
void midi1_clock_cntr_init(const struct device *midi1_dev);

/**
 * @brief getter for the internal counter frequency in MHZ
 *
 * @return frequency in MHz
 */
uint32_t midi1_clock_cntr_cpu_frequency(void);

/**
 * @brief Start periodic MIDI clock. interval_us must be > 0.
 *
 * @param interval_us interval in us must be higher than 0
 */
void midi1_clock_cntr_start(uint32_t interval_us);

/**
 * @brief Start clock with MIDI ticks as argument (more accurate)
 *
 * @param ticks tick reference to the frequency in clocks
 * @see midi1_clock_cntr_cpu_frequency(void)
 */
void midi1_clock_cntr_ticks_start(uint32_t ticks);

/**
 * @brief placeholder for updating the clock
 *
 * @note this is not supported on PIT0 channel 0 on NXP
 */
void midi1_clock_cntr_update_ticks(uint32_t new_ticks);

/**
 * @brief Stop the clock
 */
void midi1_clock_cntr_stop(void);

/**
 * @brief Generate MIDI1.0 clock
 *
 * @param midi1_dev MIDI device pointer
 * @param sbpm scaled BPM like 123.12 must be entered like 12312
 */
void midi1_clock_cntr_gen(const struct device *midi, uint16_t sbpm);

/**
 * @brief Generate MIDI1.0 clock
 *
 * @param sbpm scaled BPM like 123.12 must be entered like 12312
 */
void midi1_clock_cntr_gen_sbpm(uint16_t sbpm);

/**
 * @brief Getter for the current bpm
 *
 * @return sbpm scaled BPM like 123.12 is returned like 12312
 */
uint16_t midi1_clock_cntr_get_sbpm();


#endif /* MIDI1_CLOCK_TIMER */
/* EOF */

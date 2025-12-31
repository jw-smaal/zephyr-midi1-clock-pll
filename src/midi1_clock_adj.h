#ifndef MIDI1_CLOCK_TIMER_ADJUSTABLE
#define MIDI1_CLOCK_TIMER_ADJUSTABLE
/*
 * @brief MIDI1.0 adjustable clock generator for Zephyr RTOS.
 *
 * @note 
 * Generates MIDI Timing Clock (F8) at a configurable interval.
 * Uses k_work_delayable for precise scheduling and allows
 * runtime adjustment of the interval without restarting.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @date 20251231
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Initialize the adjustable MIDI clock subsystem.
 * Must be called once before starting the clock.
 *
 * @param midi1_dev  Pointer to the MIDI device used for sending UMP packets.
 */
void midi1_clock_adj_init(const struct device *midi1_dev);

/*
 * Start generating MIDI Timing Clock (F8) at the given interval.
 * interval_us must be > 0.
 *
 * Uses k_work_delayable for scheduling. The interval may be changed
 * later using midi1_clock_adj_set_interval_us() without stopping.
 */
void midi1_clock_adj_start(uint32_t interval_us);

/*
 * Convenience: start the clock using scaled BPM (sbpm).
 * 1.00 BPM  -> 100
 * 120.00 BPM -> 12000
 */
void midi1_clock_adj_start_sbpm(uint16_t sbpm);

/*
 * Stop generating MIDI clock. Cancels any scheduled work.
 */
void midi1_clock_adj_stop(void);

/*
 * Adjust the clock interval while running.
 * The next tick is smoothly rescheduled using k_work_reschedule().
 */
void midi1_clock_adj_set_interval_us(uint32_t interval_us);

/*
 * Convenience: adjust the interval using scaled BPM (sbpm).
 */
void midi1_clock_adj_set_sbpm(uint16_t sbpm);

/*
 * Query the currently configured interval in microseconds.
 * Returns 0 if the clock has never been started.
 */
uint32_t midi1_clock_adj_get_interval_us(void);

/*
 * Query the currently configured tempo in scaled BPM (sbpm).
 * Returns 0 if the clock has never been started.
 */
uint16_t midi1_clock_adj_get_sbpm(void);

/*
 * Returns true if the MIDI clock generator is currently active.
 */
bool midi1_clock_adj_is_running(void);

#endif				/* MIDI1_CLOCK_TIMER_ADJUSTABLE */
/* EOF */


/**
 * @file midi_clock_measure_counter.h
 * @brief MIDI 1.0 Clock BPM measurement using Zephyr counter device.
 * @details
 *  (TODO: NOT TESTED! ) 
 * Uses a free-running hardware counter to timestamp incoming MIDI Clock
 * (0xF8) pulses with microsecond precision.
 *
 * Scaled BPM representation (sbpm):
 *   1.00 BPM   -> 100
 *   100.00 BPM -> 10000
 *
 * Call midi1_clock_meas_cntr_pulse() for each received MIDI Clock tick.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251230
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_CLOCK_MEAS_CNTR_H
#define MIDI1_CLOCK_MEAS_CNTR_H

#include <stdint.h>
#include <stdbool.h>


#ifndef COUNTER_DEVICE_CH1
#define COUNTER_DEVICE_CH1 pit0_channel1
#endif

/**
 * @brief Initialize the measurement subsystem.
 * Must be called once at startup or when transport restarts.
 */
void midi1_clock_meas_cntr_init(void);

/**
 * @brief Notify the measurement module that a MIDI Clock (0xF8) pulse arrived.
 * Call this from your MIDI handler.
 */
void midi1_clock_meas_cntr_pulse(void);

/**
 * @brief Get last measured BPM in scaled form (BPM * 100).
 * @return 0 if no valid measurement yet.
 */
uint32_t midi1_clock_meas_cntr_get_sbpm(void);

/**
 * @brief Returns true if a valid BPM estimate is available.
 */
bool midi1_clock_meas_cntr_is_valid(void);

/**
 * @brief Returns the timestamp (ticks) of the last MIDI Clock tick.
 * This is used by the PLL.
 */
uint32_t midi1_clock_meas_cntr_last_timestamp(void);

uint32_t midi1_clock_meas_cntr_interval_ticks(void);

uint32_t midi1_clock_meas_cntr_interval_us(void);



#endif				/* MIDI1_CLOCK_MEAS_CNTR_H */

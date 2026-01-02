/**
 * @file midi1_clock_measure.h 
 * @brief MIDI 1.0 Clock BPM measurement (integer only, no FPU)
 * @details
 * Scaled BPM representation (sbpm):
 *   1.00 BPM   -> 100
 *   100.00 BPM -> 10000
 *
 * Designed for use with incoming MIDI Clock (0xF8) messages.
 * Call midi1_clock_meas_pulse() on each received MIDI clock tick.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251214
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_CLOCK_MEASURE_H
#define MIDI1_CLOCK_MEASURE_H

#include <stdint.h>
#include <stdbool.h>

/* Some helper functions for MIDI1.0 by J-W Smaal */
#include "midi1.h"

/**
 * @brief Initialize/reset the measurement state.
 *
 * @note Call once at startup and whenever transport is restarted
 * (MIDI Start, Stop, Continue).
 */
void midi1_clock_meas_init(void);

/**
 * @brief Notify the measurement code that a MIDI Clock (0xF8) pulse
 * was received.
 * @note Call this from your MIDI packet handler for each 0xF8.
 */
void midi1_clock_meas_pulse(void);

/**
 * @brief get current measurement in us
 * @return us interval between each received 0xF8 timing clock.
 */
uint32_t midi1_clock_meas_get_us(void);

/**
 * @brief Get the last measured BPM in scaled form (BPM * 100).
 * Example: return value 12345 means 123.45 BPM.
 *
 * @return 0 if no valid measurement is available yet.
 */
uint16_t midi1_clock_meas_get_sbpm(void);

/**
 * @brief Returns true if the measurement has a valid BPM estimate.
 * @note This will become true after at least two MIDI clock pulses were
 *  observed.
 * @return true if the measurement has a valid BPM estimate
 */
bool midi1_clock_meas_is_valid(void);

/**
 * @brief return the last measured interval
 * @return interval in us
 */
uint32_t midi1_clock_meas_last_interval(void);

/**
 * @brief return the last timestamp
 * @return free‑running 32‑bit timestamp in microseconds.
 */
uint32_t midi1_clock_meas_last_timestamp(void);

/*------------------------------------------------------------------- */
#endif				/* MIDI1_CLOCK_MEASURE_H */
/* EOF */

/*
 * MIDI 1.0 Clock BPM measurement (integer only, no FPU)
 *
 * Scaled BPM representation (sbpm):
 *   1.00 BPM   -> 100
 *   100.00 BPM -> 10000
 *
 * Designed for use with incoming MIDI Clock (0xF8) messages.
 * Call midi1_clock_meas_pulse() on each received MIDI clock tick.
 *
 * By Jan-Willem Smaal <usenet@gispen.org>
 * 2025-12-14
 */

#ifndef MIDI1_CLOCK_MEASURE_H
#define MIDI1_CLOCK_MEASURE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Initialize/reset the measurement state.
 * Call once at startup and whenever transport is restarted (MIDI Start, Stop, Continue).
 */
void midi1_clock_meas_init(void);

/*
 * Notify the measurement code that a MIDI Clock (0xF8) pulse was received.
 * Call this from your MIDI packet handler for each 0xF8.
 */
void midi1_clock_meas_pulse(void);

/*
 * Get the last measured BPM in scaled form (BPM * 100).
 * Example: return value 12345 means 123.45 BPM.
 *
 * Returns 0 if no valid measurement is available yet.
 */
uint32_t midi1_clock_meas_get_sbpm(void);

/*
 * Returns true if the measurement has a valid BPM estimate.
 * This will become true after at least two MIDI clock pulses were observed.
 */
bool midi1_clock_meas_is_valid(void);

#endif /* MIDI1_CLOCK_MEASURE_H */

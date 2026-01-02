
/**
 * @file midi_clock_measure_counter.h
 * @brief MIDI 1.0 Clock BPM measurement using Zephyr counter device.
 * @details

 * Uses a free-running hardware counter to timestamp incoming MIDI Clock
 * (0xF8) pulses with microsecond precision. uses PIT0 channel 1

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


/**
 * @note We are using PIT0 channel 1 one the FRDM MCXC242 development board
 * In the device-tree overlay make sure it's enabled !
 *
 * &pit0 {
 *	status = "okay";
 * };
 *
 */
#ifndef COUNTER_DEVICE_CH1
#define COUNTER_DEVICE_CH1 pit0_channel1
#endif

/**
 * @brief Initialize the measurement subsystem.
 *
 * @note Must be called once at startup or when transport restarts.
 */
void midi1_clock_meas_cntr_init(void);

/**
 * @brief Notify the measurement module that a MIDI Clock (0xF8) pulse arrived.
 *
 * @note Call this from your MIDI handler.
 */
void midi1_clock_meas_cntr_pulse(void);

/**
 * @brief Get last measured BPM in scaled form (BPM * 100).
 * @return 0 if no valid measurement yet.
 */
uint32_t midi1_clock_meas_cntr_get_sbpm(void);

/**
 * @brief Returns true if a valid BPM estimate is available.
 *
 * @return true if measurement is valid
 * @return false if measurement is not available or invalid
 */
bool midi1_clock_meas_cntr_is_valid(void);

/**
 * @brief Returns the timestamp (ticks) when the  MIDI Clock tick.
 * was received
 * This can be used by the PLL e.g.
 * @return timestamp in ticks (decreasing if using PIT0 channel1)
 */
uint32_t midi1_clock_meas_cntr_last_timestamp(void);

/**
 * @brief Returns the interval (ticks) when the  MIDI Clock tick.
 * was received compared to the previous one.
 *
 * @return tick interval in ticks
 */
uint32_t midi1_clock_meas_cntr_interval_ticks(void);

/**
 * @brief Returns the interval us when the  MIDI Clock tick.
 * was received compared to the previous one.
 *
 * @return tick interval in us
 */
uint32_t midi1_clock_meas_cntr_interval_us(void);

#endif /* MIDI1_CLOCK_MEAS_CNTR_H */
/* EOF */

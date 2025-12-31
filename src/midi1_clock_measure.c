/**
 * @brief MIDI 1.0 Clock BPM measurement (integer only, no FPU)
 * @note
 * Scaled BPM representation (sbpm):
 *   1.00 BPM   -> 100
 *   100.00 BPM -> 10000
 *
 * @note Implementation notes:
 * @code
 *   - Uses Zephyr's cycle counter and k_cyc_to_us_floor32() for timing.
 *   - Pure integer math, no floating point, safe on ARM M0+.
 *   - Formula:
 *       BPM        = 60 / (24 * T_pulse)
 *       T_pulse    = interval between MIDI clock pulses [seconds]
 *       scaledBPM  = BPM * 100
 *
 *     With T_pulse in microseconds:
 *       scaledBPM = (60 * 1_000_000 * 100) / (24 * interval_us)
 *                 = 250000000 / interval_us
 * @endcode
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251214
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "midi1_clock_measure.h"

/* Some helper functions for MIDI1.0 by J-W Smaal */
#include "midi1.h"

/* Internal state */
static uint32_t g_last_ts_us;
static uint32_t g_last_interval_us;
static uint16_t g_scaled_bpm;
static bool g_valid;

/**
 * @code
 * Constant derived from:
 * scaledBPM = (60 seconds * 1_000_000 us * 100sbpm *) / (24 * interval_us)
 *           = 250000000 / interval_us
 *
 *
 * so there are defined in "midi1.h"
 * #define BPM_SCALE      100u
 * #define US_PER_SECOND  1000000u
 * @endcode
 * 
 * Normally this should be:   250000000u
 * and the result can be stored in a uint32_t
 * however the intermediate value cannot fit into uint32_t
 * that is why I am using "ull" during compile time.
 */
#define MIDI1_SCALED_BPM_NUMERATOR ((60ull * US_PER_SECOND * BPM_SCALE)/24ull)

void midi1_clock_meas_init(void)
{
	g_last_ts_us = 0u;
	g_last_interval_us = 0u;
	g_scaled_bpm = 0u;
	g_valid = false;
}

/*
 * Get current timestamp in microseconds.
 * Uses Zephyr's cycle counter and conversion helper.
 */
static inline uint32_t midi1_clock_meas_get_us(void)
{
	/*
	 * TODO:
	 * need to work on a free running clock implementation.
	 */
	uint32_t cycles = k_cycle_get_32();
	return k_cyc_to_us_floor32(cycles);
}

void midi1_clock_meas_pulse(void)
{
	
	uint32_t now_us = midi1_clock_meas_get_us();
	
	if (g_last_ts_us != 0u) {
		/* wrap-safe in uint32_t */
		uint32_t interval_us = now_us - g_last_ts_us;
		g_last_interval_us = interval_us;
		if (interval_us > 0u) {
			uint32_t sbpm =
			    MIDI1_SCALED_BPM_NUMERATOR / interval_us;
			g_scaled_bpm = sbpm;
			g_valid = true;
		}
	}
	g_last_ts_us = now_us;
	return;
}

uint16_t midi1_clock_meas_get_sbpm(void)
{
	if (!g_valid) {
		return 0u;
	}
	return g_scaled_bpm;
}

bool midi1_clock_meas_is_valid(void)
{
	return g_valid;
}

uint32_t midi1_clock_meas_last_interval(void)
{
	return g_last_interval_us;
}

uint32_t midi1_clock_meas_last_timestamp(void)
{
	return g_last_ts_us;
}

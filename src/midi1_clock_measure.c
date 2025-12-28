/*
 * MIDI 1.0 Clock BPM measurement (integer only, no FPU)
 *
 * Scaled BPM representation (sbpm):
 *   1.00 BPM   -> 100
 *   100.00 BPM -> 10000
 *
 * Implementation notes:
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
 *
 * By Jan-Willem Smaal <usenet@gispen.org>
 * 2025-12-14
 */

#include "midi1_clock_measure.h"

#include <zephyr/kernel.h>

/* Internal state */
static uint32_t g_last_ts_us;
static uint32_t g_scaled_bpm;
static bool     g_valid;

/* Constant derived from:
 * scaledBPM = (60 * 1_000_000 * 100) / (24 * interval_us)
 *           = 250000000 / interval_us
 */
#define MIDI1_SCALED_BPM_NUMERATOR  250000000u

void midi1_clock_meas_init(void)
{
	g_last_ts_us = 0u;
	g_scaled_bpm = 0u;
	g_valid      = false;
}

/*
 * Get current monotonic timestamp in microseconds.
 * Uses Zephyr's cycle counter and conversion helper.
 */
static inline uint32_t midi1_clock_meas_get_us(void)
{
	uint32_t cycles = k_cycle_get_32();
	return k_cyc_to_us_floor32(cycles);
}

void midi1_clock_meas_pulse(void)
{
	uint32_t now_us = midi1_clock_meas_get_us();
	
	if (g_last_ts_us != 0u) {
		uint32_t interval_us = now_us - g_last_ts_us; /* wrap-safe in uint32_t */
		
		if (interval_us > 0u) {
			uint32_t sbpm = MIDI1_SCALED_BPM_NUMERATOR / interval_us;
			
			/* Optional simple smoothing:
			 * g_scaled_bpm = (g_scaled_bpm * 3u + sbpm) / 4u;
			 * For now, just take the latest value.
			 */
			g_scaled_bpm = sbpm;
			g_valid = true;
		}
	}
	
	g_last_ts_us = now_us;
}

uint32_t midi1_clock_meas_get_sbpm(void)
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

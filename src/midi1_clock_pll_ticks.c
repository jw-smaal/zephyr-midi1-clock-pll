/**
 * @file midi1_clock_pll_ticks.c
 * @brief Simple integer PLL for MIDI clock synchronization (24 PPQN).
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251229
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
/* For printk only */
#include <zephyr/kernel.h>

#include "midi1_clock_pll_ticks.h"
#include "midi1.h"		/* my sbpm_to_us_interval() */

/* Loop filter constants */

/*
 * Low‑pass filter strength keep it high... sudden tempo changes
 * should be followed though.
 */
#define MIDI1_PLL_FILTER_K  16

/*
 * Correction gain keep it low we want to move towards the value
 * but not overshoot
 */
#define MIDI1_PLL_GAIN_G    8

/*
 * Slow loop tracking gain.
 */
#define MIDI1_PLL_TRACK_H 512
#define MIDI1_PLL_TRACK_GAIN 512


#define DEBUG_PLL 0

static uint32_t midi1_nominal_interval_ticks;
static int32_t midi1_internal_interval_ticks;
//static uint32_t midi1_next_expected_ticks;
static int32_t midi1_filtered_error = 0;
static int32_t midi1_slow_error_accum;

/* TODO: must return PIT ticks, not microseconds */
//extern uint32_t sbpm_to_24pqn_ticks(uint16_t sbpm);

void midi1_pll_ticks_init(uint16_t sbpm)
{
	//midi1_nominal_interval_ticks  = sbpm_to_24pqn_ticks(sbpm);
	// TODO: implement now set a static value
	midi1_nominal_interval_ticks  = 503000;
	midi1_internal_interval_ticks = (int32_t)midi1_nominal_interval_ticks;
	midi1_filtered_error          = 0;
	midi1_slow_error_accum 	      = 0;
}

/* measured_interval_ticks is the last "interval measured as: X" */
void midi1_pll_ticks_process_interval(uint32_t measured_interval_ticks)
{
	if (measured_interval_ticks == 0U) {
		/* ignore bogus measurement */
		return;
	}
	
	/* 1. Interval error: measured - internal */
	int32_t error =
	(int32_t)measured_interval_ticks - midi1_internal_interval_ticks;
	
	/* 2. Low-pass filter the error */
	midi1_filtered_error +=
	(error - midi1_filtered_error) / MIDI1_PLL_FILTER_K;
	
	/* 3. Adjust internal interval around nominal */
	midi1_internal_interval_ticks =
	(int32_t)midi1_nominal_interval_ticks +
	midi1_filtered_error / MIDI1_PLL_GAIN_G;
	
#if 0
	
	/* Clamp to sane bounds: e.g., ±50% around nominal */
	int32_t min_ticks = (int32_t)midi1_nominal_interval_ticks / 2;
	int32_t max_ticks = (int32_t)midi1_nominal_interval_ticks * 2;
	
	if (midi1_internal_interval_ticks < min_ticks) {
		midi1_internal_interval_ticks = min_ticks;
	} else if (midi1_internal_interval_ticks > max_ticks) {
		midi1_internal_interval_ticks = max_ticks;
	}
#endif
	
	/* Slow tracking: adapt nominal over time */
	midi1_slow_error_accum += midi1_filtered_error;
	
	/* When slow_error_accum gets big enough, nudge nominal */
	if (midi1_slow_error_accum > MIDI1_PLL_TRACK_H) {
		midi1_nominal_interval_ticks++;
		midi1_slow_error_accum -= MIDI1_PLL_TRACK_H;
	} else if (midi1_slow_error_accum < -MIDI1_PLL_TRACK_H) {
		midi1_nominal_interval_ticks--;
		midi1_slow_error_accum += MIDI1_PLL_TRACK_H;
	}

	/* Slow tracking: adapt nominal interval towards long-term average.
	 *
	 * We add a small fraction of the filtered error each pulse.
	 * This makes nominal_interval_ticks follow real BPM over time.
	 */
	//midi1_nominal_interval_ticks +=
	//(int32_t)midi1_filtered_error / MIDI1_PLL_TRACK_GAIN;
	
#if DEBUG_PLL
	printk("PLL meas=%u  err=%d  filt=%d  int=%d error_accum=%d nominal=%d\n",
	       measured_interval_ticks,
	       error,
	       midi1_filtered_error,
	       midi1_internal_interval_ticks,
	       midi1_slow_error_accum,
	       midi1_nominal_interval_ticks);
#endif
}

int32_t midi1_pll_ticks_get_interval_ticks(void)
{
	return midi1_nominal_interval_ticks;
}

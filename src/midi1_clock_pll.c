/**
 * @file midi1_clock_pll.c
 * @brief Simple integer PLL for MIDI clock synchronization (24 PPQN).
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251229
 * @license SPDX-License-Identifier: Apache-2.0
 */

#include "midi1_clock_pll.h"
#include "midi1.h"		/* my sbpm_to_us_interval() */

/* Loop filter constants */
#define MIDI1_PLL_FILTER_K   20	/* Low‑pass filter strength */
#define MIDI1_PLL_GAIN_G     8	/* Correction gain */

static uint32_t midi1_nominal_interval_us;
static uint32_t midi1_internal_interval_us;
static uint32_t midi1_next_expected_us;

static int32_t midi1_filtered_error = 0;

void midi1_pll_init(uint16_t sbpm)
{
	midi1_nominal_interval_us = sbpm_to_us_interval(sbpm);
	midi1_internal_interval_us = midi1_nominal_interval_us;
	midi1_next_expected_us = 0;
	midi1_filtered_error = 0;
}

void midi1_pll_process_tick(uint32_t t_in_us)
{
	if (midi1_next_expected_us == 0) {
		/* First tick: initialize phase reference */
		midi1_next_expected_us = t_in_us + midi1_internal_interval_us;
		return;
	}

	/* 1. Phase error: how early/late is the incoming tick? */
	int32_t phase_error =
	    (int32_t) t_in_us - (int32_t) midi1_next_expected_us;

	/* 2. Low‑pass filter the phase error */
	midi1_filtered_error +=
	    (phase_error - midi1_filtered_error) / MIDI1_PLL_FILTER_K;

	/* 3. Adjust internal interval */
	int32_t correction = midi1_filtered_error / MIDI1_PLL_GAIN_G;
	midi1_internal_interval_us = midi1_nominal_interval_us + correction;

	/* 4. Advance expected timestamp */
	midi1_next_expected_us += midi1_internal_interval_us;
}

uint32_t midi1_pll_get_interval_us(void)
{
	return midi1_internal_interval_us;
}

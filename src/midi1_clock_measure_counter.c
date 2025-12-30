
/*
 * @file midi_clock_measure_counter.c
 * @brief MIDI 1.0 Clock BPM measurement using Zephyr counter device.
 * Hardware-accurate
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @license SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>

#include "midi1.h"
#include "midi1_clock_meas_cntr.h"

/* ------------------------------------------------------------------ */
/* Internal state */

static const struct device *g_counter_dev = NULL;

static uint32_t g_last_ts_us = 0;
static uint32_t g_scaled_bpm = 0;
static bool g_valid = false;

/* Timestamp exposed to PLL */
static uint32_t g_last_tick_timestamp_us = 0;

/* ------------------------------------------------------------------ */
/*
 * Numerator:
 * scaledBPM = (60 * 1_000_000 * 100) / (24 * interval_us)
 *           = 250000000 / interval_us
 */
#define MIDI1_SCALED_BPM_NUMERATOR ((60ull * US_PER_SECOND * BPM_SCALE) / 24ull)

/* ------------------------------------------------------------------ */
/* Read free-running counter and convert to microseconds */
static inline uint32_t meas_now_us(void)
{
	uint32_t ticks = 0;
	
	int err = counter_get_value(g_counter_dev, &ticks);
	if (err != 0) {
		return 0;
	}
	
	return counter_ticks_to_us(g_counter_dev, ticks);
}

/* ------------------------------------------------------------------ */
void midi1_clock_meas_cntr_init(void)
{
	g_last_ts_us = 0;
	g_scaled_bpm = 0;
	g_valid = false;
	
	g_counter_dev = DEVICE_DT_GET(DT_NODELABEL(COUNTER_DEVICE));
	if (!device_is_ready(g_counter_dev)) {
		printk("Clock measurement counter device not ready\n");
		return;
	}
	
	/* Start free-running counter */
	int err = counter_start(g_counter_dev);
	if (err != 0) {
		printk("Failed to start measurement counter: %d\n", err);
	}
}

/* ------------------------------------------------------------------ */
void midi1_clock_meas_cntr_pulse(void)
{
	uint32_t now_us = meas_now_us();
	g_last_tick_timestamp_us = now_us;
	
	if (g_last_ts_us != 0) {
		uint32_t interval_us = now_us - g_last_ts_us; /* wrap-safe */
		
		if (interval_us > 0) {
			uint32_t sbpm =
			MIDI1_SCALED_BPM_NUMERATOR / interval_us;
			
			g_scaled_bpm = sbpm;
			g_valid = true;
		}
	}
	
	g_last_ts_us = now_us;
}

/* ------------------------------------------------------------------ */
uint32_t midi1_clock_meas_cntr_get_sbpm(void)
{
	return g_valid ? g_scaled_bpm : 0;
}

bool midi1_clock_meas_cntr_is_valid(void)
{
	return g_valid;
}

uint32_t midi1_clock_meas_cntr_last_timestamp(void)
{
	return g_last_tick_timestamp_us;
}

/* EOF */

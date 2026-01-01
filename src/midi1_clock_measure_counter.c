
/*
 * @file midi_clock_measure_counter.c
 * @brief MIDI 1.0 Clock BPM measurement using Zephyr counter device.    (TODO: NOT TESTED! ) 
 * Hardware-accurate
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @license SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>

#include "midi1.h"
#include "midi1_clock_measure_counter.h"

/* ------------------------------------------------------------------ */
/* Internal state */

static const struct device *g_counter_dev_ch1 = NULL;

static uint32_t g_last_ts_ticks = 0;
static uint32_t g_scaled_bpm = 0;
static uint32_t g_last_interval_ticks = 0;
static bool g_valid = false;

/* Timestamp exposed to PLL */
static uint32_t g_last_tick_timestamp_ticks = 0;

/* ------------------------------------------------------------------ */
/*
 * Numerator:
 * scaledBPM = (60 * 1_000_000 * 100) / (24 * interval_us)
 *           = 250000000 / interval_us
 */
#define MIDI1_SCALED_BPM_NUMERATOR ((60ull * US_PER_SECOND * BPM_SCALE) / 24ull)

/* ------------------------------------------------------------------ */
/* Read free-running counter and convert to microseconds */
static inline uint32_t midi1_clock_meas_now_ticks(void)
{
	uint32_t ticks = 0;

	int err = counter_get_value(g_counter_dev_ch1, &ticks);
	if (err != 0) {
		printk("counter_get_value error\n");
		return 0;
	}
	//printk("number of ticks:%u\n", ticks);
	/* Is decreasing over time */
	return ticks;
}

/* ------------------------------------------------------------------ */
/* TODO: has a BUG crashes when counter wraps around maybe unhandled IRQ */
void midi1_clock_meas_cntr_init(void)
{
	g_last_ts_ticks = 0;
	g_scaled_bpm = 12000;
	g_last_interval_ticks = 0;
	g_valid = false;

	g_counter_dev_ch1 = DEVICE_DT_GET(DT_NODELABEL(COUNTER_DEVICE_CH1));
	if (!device_is_ready(g_counter_dev_ch1)) {
		printk("Clock measurement counter device not ready\n");
		return;
	}

	/* Start free-running counter */
	int err = counter_start(g_counter_dev_ch1);
	if (err != 0) {
		printk("Failed to start measurement counter: %d\n", err);
	}
	/* Initialize last timestamp to current counter value (down-counter) */
	g_last_ts_ticks = midi1_clock_meas_now_ticks();
}

/* ------------------------------------------------------------------ */
void midi1_clock_meas_cntr_pulse_BROKEN(void)
{
	uint32_t now_ticks = midi1_clock_meas_now_ticks();
	/* for the PLL to read e.g. */
	g_last_tick_timestamp_ticks = now_ticks;

	if (g_last_ts_ticks != 0) {
		g_last_interval_ticks = g_last_ts_ticks - now_ticks;
		if (g_last_interval_ticks > 0) {
			uint32_t sbpm =
			    MIDI1_SCALED_BPM_NUMERATOR / midi1_clock_meas_cntr_interval_us();
			g_scaled_bpm = sbpm;
			g_valid = true;
		}
	}
	g_last_ts_ticks = now_ticks;
}

void midi1_clock_meas_cntr_pulse(void)
{
	uint32_t now_ticks = midi1_clock_meas_now_ticks();
	
	/* Expose timestamp to PLL or other users */
	g_last_tick_timestamp_ticks = now_ticks;
	
	/* First pulse after init: we have no previous timestamp yet */
	if (g_last_ts_ticks == 0U) {
		g_last_ts_ticks = now_ticks;
		return;
	}
	
	/* For a down-counter, elapsed = previous - current (unsigned wrap-safe) */
	uint32_t interval_ticks = g_last_ts_ticks - now_ticks;
	g_last_ts_ticks = now_ticks;
	
	/* Reject zero or obviously bogus intervals to avoid BPM math crashes */
	if (interval_ticks == 0U) {
		return;
	}
	
	g_last_interval_ticks = interval_ticks;
	
	uint32_t interval_us = midi1_clock_meas_cntr_interval_us();
	if (interval_us == 0U) {
		return;
	}
	
	uint32_t sbpm = MIDI1_SCALED_BPM_NUMERATOR / interval_us;
	g_scaled_bpm = sbpm;
	g_valid = true;
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
	return g_last_tick_timestamp_ticks;
}

uint32_t midi1_clock_meas_cntr_interval_ticks(void)
{
	return g_last_interval_ticks;
}


uint32_t midi1_clock_meas_cntr_interval_us(void)
{
	return counter_ticks_to_us(g_counter_dev_ch1, midi1_clock_meas_cntr_interval_ticks());
}








/* EOF */

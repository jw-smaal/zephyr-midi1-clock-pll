
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
#include "midi1_blockavg.h"

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

/*
 * defined but doing nothing I could not find the correct way to
 * switch off the IRQ with the top_cfg...
 */
void midi1_clock_meas_callback(void)
{
	//printk("midi1_clock_meas_callback fired!\n");
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

	/* Do this once and then let it run free no calbacks etc.. */
	const struct counter_top_cfg top_cfg = {
		.ticks = 0xFFFFFFFF,   /* full 32‑bit range */
		.callback = (void *)midi1_clock_meas_callback,
		//.callback = 0,
		.user_data = 0,
		.flags = 0,
	};
	counter_set_top_value(g_counter_dev_ch1, &top_cfg);
	
	
	/* Start free-running counter */
	int err = counter_start(g_counter_dev_ch1);
	if (err != 0) {
		printk("Failed to start measurement counter: %d\n", err);
	}
	/* Initialize last timestamp to current counter value (down-counter) */
	g_last_ts_ticks = midi1_clock_meas_now_ticks();
}

/* ------------------------------------------------------------------ */
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
	
	/*
	 * For a down-counter,
	 * elapsed = previous - current (unsigned wrap-safe)
	 */
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
	
	/*
	 * Let average the BPM over 24 clock's 0xF8 received otherwise
	 * it goes all over the place
	 */
	midi1_blockavg_add(interval_ticks);
	
	if (midi1_blockavg_count() == MIDI1_BLOCKAVG_SIZE) {
		uint32_t avg_ticks = midi1_blockavg_average();
		uint32_t interval_us = counter_ticks_to_us(g_counter_dev_ch1, avg_ticks);
		g_scaled_bpm = MIDI1_SCALED_BPM_NUMERATOR / interval_us;
		g_valid = true;
	}
	
	/* old code */
#if 0
	uint32_t sbpm = MIDI1_SCALED_BPM_NUMERATOR / interval_us;
	g_scaled_bpm = sbpm;
	g_valid = true;
#endif
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

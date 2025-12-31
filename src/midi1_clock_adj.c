/**
 *
 * @brief Adjustable MIDI1.0 Clock Generator for Zephyr RTOS
 * using k_work_delayable for smooth interval changes.
 *
 * Uses:
 *   - sbpm_to_us_interval()
 *   - us_interval_to_sbpm()
 *   - midi1_timing_clock()
 *   - usbd_midi_send()
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251231
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

#include "midi1_clock_adj.h"
#include "midi1.h"		/* tempo helpers + midi1_timing_clock() */


/*
 * This is part of the MIDI2 library prj.conf
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h
 * it gets linked in and is required for the USB MIDI support.
 */
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <ump_stream_responder.h>

#define MIDI_CLOCK_ON_PIN 1
#if MIDI_CLOCK_ON_PIN
/*
 * To measure MIDI clock externally we toggle a PIN and measure with
 * the oscilloscope
 */
#include <zephyr/drivers/gpio.h>
#endif


/* -------------------------------------------------------------------------- */
/* Internal state                                                             */
/* -------------------------------------------------------------------------- */
static const struct device *g_midi_dev = NULL;

static struct k_work_delayable g_clk_work;

/* atomic interval in microseconds */
static atomic_t g_interval_us = ATOMIC_INIT(0);

/* cached sbpm */
static uint16_t g_sbpm = 0;

/* running flag */
static atomic_t g_running = ATOMIC_INIT(0);


/* -------------------------------------------------------------------------- */
/* MIDI send glue                                                             */
/* -------------------------------------------------------------------------- */
static inline void midi1_clock_send_tick(const struct device *dev)
{
	if (dev) {
		usbd_midi_send(dev, midi1_timing_clock());
		//printk(".");
	}
}


/* -------------------------------------------------------------------------- */
/* Work handler                                                               */
/* -------------------------------------------------------------------------- */
static void midi1_clk_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!atomic_get(&g_running)) {
		return;
	}

	/* 1. Send MIDI Clock (F8) */
	midi1_clock_send_tick(g_midi_dev);

	/* 2. Reschedule next tick */
	uint32_t interval_us = (uint32_t) atomic_get(&g_interval_us);

	if (interval_us > 0u) {
		k_work_reschedule(&g_clk_work, K_USEC(interval_us));
	}
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

void midi1_clock_adj_init(const struct device *midi1_dev)
{
	g_midi_dev = midi1_dev;

	atomic_set(&g_interval_us, 0);
	atomic_set(&g_running, 0);
	g_sbpm = 0;

	k_work_init_delayable(&g_clk_work, midi1_clk_work_handler);
}

void midi1_clock_adj_start(uint32_t interval_us)
{
	if (interval_us == 0u) {
		return;
	}

	atomic_set(&g_interval_us, (atomic_val_t) interval_us);
	g_sbpm = pqn24_to_sbpm(interval_us);

	atomic_set(&g_running, 1);

	k_work_schedule(&g_clk_work, K_USEC(interval_us));
}

void midi1_clock_adj_start_sbpm(uint16_t sbpm)
{
	// Removed: we need 24 pulses per quater note
	//midi1_clock_adj_start(sbpm_to_us_interval(sbpm));
	midi1_clock_adj_start(sbpm_to_24pqn(sbpm));
}

void midi1_clock_adj_stop(void)
{
	atomic_set(&g_running, 0);
	k_work_cancel_delayable(&g_clk_work);
}

void midi1_clock_adj_set_interval_us(uint32_t interval_us)
{
	if (interval_us == 0u) {
		return;
	}

	atomic_set(&g_interval_us, (atomic_val_t) interval_us);
	// Removed the interval is in 24pqn not us
	// g_sbpm = us_interval_to_sbpm(interval_us);
	g_sbpm = pqn24_to_sbpm(interval_us);
	
	if (atomic_get(&g_running)) {
		k_work_reschedule(&g_clk_work, K_USEC(interval_us));
	}
}

void midi1_clock_adj_set_sbpm(uint16_t sbpm)
{
	midi1_clock_adj_set_interval_us(sbpm_to_24pqn(sbpm));
}

uint32_t midi1_clock_adj_get_interval_us(void)
{
	return (uint32_t) atomic_get(&g_interval_us);
}

uint16_t midi1_clock_adj_get_sbpm(void)
{
	return g_sbpm;
}

bool midi1_clock_adj_is_running(void)
{
	return atomic_get(&g_running) != 0;
}

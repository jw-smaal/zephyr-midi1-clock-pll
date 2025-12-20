/*
 * implementation of midi1_clock_clock by Jan-Willem Smaal <usenet@gispen.org
 * this is a hardware based counter tested with NXP FRDM_MCXC242 in zephyr.
 * 20251214
 */
#include <zephyr/audio/midi.h>
#include <zephyr/drivers/counter.h>

/* This is part of the MIDI2 library prj.conf
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h it get's linked in.
 */
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <ump_stream_responder.h>

/* MIDI helpers by J-W Smaal*/
#include "midi1.h"
#include "midi1_clock_counter.h"

static atomic_t g_midi1_running_cntr = ATOMIC_INIT(0);
static struct device *g_midi1_dev; 
const struct device *g_counter_dev;

/* This is the ISR/callback */ 
static void midi1_cntr_handler(const struct device *dev, void *midi1_dev_arg)
{
	//g_midi1_dev = (struct device *)midi1_dev_arg;

	if (!atomic_get(&g_midi1_running_cntr)) {
		return;
	}
	if (g_midi1_dev) {
		usbd_midi_send(g_midi1_dev, midi1_timing_clock());
	}
}

/*
 * Initialize MIDI clock subsystem with your MIDI device handle. Call once at
 * startup before starting the clock.
 */
void midi1_clock_cntr_init(const struct device *midi1_dev_arg)
{
	int err = 0;

	atomic_set(&g_midi1_running_cntr, 0);
	g_counter_dev = DEVICE_DT_GET(DT_NODELABEL(COUNTER_DEVICE));
	if (!device_is_ready(g_counter_dev)) {
		printk("Counter device not ready\n");
		return;
	}
	g_midi1_dev = midi1_dev_arg;
	return;
}

/*
 * Start periodic MIDI clock. interval_us must be > 0. Uses k_timer_start with
 * the same period for initial and repeat time.
 */
void midi1_clock_cntr_start(uint32_t interval_us)
{
	int err = 0;
	if (interval_us == 0u) {
		return;
	}
	atomic_set(&g_midi1_running_cntr, 1);

	uint32_t ticks = counter_us_to_ticks(g_counter_dev, interval_us); 


	/* Configure top value: e.g. 1 second at 48 MHz clock */
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = g_midi1_dev,
		//.ticks = 48000000U,
		.ticks = ticks,
		.flags = 0,
	};

	err = counter_set_top_value(g_counter_dev, &top_cfg);
	if (err != 0) {
		printk("Failed to set top value: %d\n", err);
		return;
	}

	/* Start the counter */
	err = counter_start(g_counter_dev);
	if (err != 0) {
		printk("Failed to start counter: %d\n", err);
		return;
	}
}

/* Stop the clock */
void midi1_clock_cntr_stop(void)
{
	atomic_set(&g_midi1_running_cntr, 0);
	//k_timer_stop(&g_midi1_timer);
}

/* EOF */

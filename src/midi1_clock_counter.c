/**
 * @file midi1_clock_counter.c
 * @brief implementation of midi1_clock_clock
 * this is a _hardware_ based counter PIT0 channel 0
 * tested with NXP FRDM_MCXC242 in zephyr.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org
 * @date 20251214
 * @license SPDX-License-Identifier: Apache-2.0
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

#define MIDI_CLOCK_ON_PIN 1

#if MIDI_CLOCK_ON_PIN
/*
 * To measure MIDI clock externally we toggle a PIN and measure with
 * the oscilloscope
 */
#include <zephyr/drivers/gpio.h>
#endif 


/* MIDI helpers by J-W Smaal*/
#include "midi1.h"
#include "midi1_clock_counter.h"

static atomic_t g_midi1_running_cntr = ATOMIC_INIT(0);
static uint16_t g_sbpm = 0;
static const struct device *g_midi1_dev;
const struct device *g_counter_dev;



/*
 * MIDI clock measurement on a PIN.
 * I used PTC8 on the FRDM_MCXC242 scope confirms correct implementation.  
 */
#if MIDI_CLOCK_ON_PIN
#define CLOCK_FREQ_OUT DT_NODELABEL(freq_out)
static const struct gpio_dt_spec clock_pin = GPIO_DT_SPEC_GET(CLOCK_FREQ_OUT, gpios);

static void midi1_debug_gpio_init(void)
{
	int ret = gpio_pin_configure_dt(&clock_pin, GPIO_OUTPUT_INACTIVE); 
	if (ret < 0){
		printk("Error configing pin\n");
		return; 
	}
}
#endif 

/* 
 * This is the ISR/callback TODO: check if usbd_midi_send is non-blocking 
 */ 
static void midi1_cntr_handler(const struct device *dev, void *midi1_dev_arg)
{
#if MIDI_CLOCK_ON_PIN
	gpio_pin_toggle_dt(&clock_pin);
#endif

	if (!atomic_get(&g_midi1_running_cntr)) {
		return;
	}
	if (g_midi1_dev) {
		usbd_midi_send(g_midi1_dev, midi1_timing_clock());
	}
	return; 
}

uint32_t midi1_clock_cntr_cpu_frequency(void) 
{
	return counter_get_frequency(g_counter_dev); 
}

/*
 * Initialize MIDI clock subsystem with the MIDI device handle. Call
 * once at startup before starting the clock.
 */
void midi1_clock_cntr_init(const struct device *midi1_dev_arg)
{
	atomic_set(&g_midi1_running_cntr, 0);
	g_counter_dev = DEVICE_DT_GET(DT_NODELABEL(COUNTER_DEVICE));
	if (!device_is_ready(g_counter_dev)) {
		printk("Counter device not ready\n");
		return;
	}
	g_midi1_dev = midi1_dev_arg;
#if MIDI_CLOCK_ON_PIN
	midi1_debug_gpio_init();
#endif 

	return;
}


/*
 * Start periodic MIDI clock. ticks must be > 0.
 */
void midi1_clock_cntr_ticks_start(uint32_t ticks)
{
	int err = 0;
	if (ticks == 0u) {
		return;
	}
	atomic_set(&g_midi1_running_cntr, 1);
#if MIDI_CLOCK_ON_PIN
	//printk("Ticks requested: %u\n", ticks);
#endif
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *) g_midi1_dev,
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

/* TODO: is not working with PIT!! BUG in my thinking */
void midi1_clock_cntr_update_ticks(uint32_t new_ticks)
{
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *)g_midi1_dev,
		.ticks = new_ticks,
		.flags = COUNTER_TOP_CFG_DONT_RESET,   /* <-- KEY */
	};
	
	int err = counter_set_top_value(g_counter_dev, &top_cfg);
	if (err != 0) {
		printk("Failed to set top value: %d\n", err);
		return;
	}
	//printk("Updating ticks to: %u\n", new_ticks);
}

/*
 * Start periodic MIDI clock. interval_us must be > 0. 
 */
void midi1_clock_cntr_start(uint32_t interval_us)
{
	int err = 0;
	if (interval_us == 0u) {
		return;
	}
	atomic_set(&g_midi1_running_cntr, 1);

	uint32_t ticks = counter_us_to_ticks(g_counter_dev, interval_us); 
	g_sbpm = us_interval_to_sbpm(interval_us);

	/*
	 * Configure top value when it overflows the midi1_cntr_handler is
	 * called as an ISR
	 */
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *) g_midi1_dev,
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
}


/**
 * @brief Generate MIDI1.0 clock
 */
void midi1_clock_cntr_gen(const struct device *midi_ptr, uint16_t sbpm) {
	midi1_clock_cntr_stop();
	midi1_clock_cntr_init(midi_ptr);
	
	uint32_t ticks = sbpm_to_ticks(
				       sbpm,
				       midi1_clock_cntr_cpu_frequency()
				       );
	
	midi1_clock_cntr_ticks_start(ticks);
}


void midi1_clock_cntr_gen_sbpm(uint16_t sbpm) {
	uint32_t ticks = sbpm_to_ticks(
				sbpm,
				midi1_clock_cntr_cpu_frequency());
	
	midi1_clock_cntr_ticks_start(ticks);
}

uint16_t midi1_clock_cntr_get_sbpm() {
	return g_sbpm;
}


/* EOF */

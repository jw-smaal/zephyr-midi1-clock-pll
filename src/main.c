/**
 * @brief MIDI 1.0 into Universal MIDI Packet over USB by J-W Smaal
 * using a sensor value's to send MIDI1.0 encapsulated into UMP
 * over USB. Doing various things such as measure MIDI clock.
 * generate a stable MIDI clock send some control changes etc...
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 *
 * ---
 * Adapted Original: Sample application for USB MIDI 2.0 device class
 * @author Copyright (c) 2024 Titouan Christophe
 *
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>

/*
 * This is part of the MIDI2 library prj.conf
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h
 * it gets linked in and is required for the USB MIDI support.
 */
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <ump_stream_responder.h>

/*
 * Functions for MIDI1 encapulation into UMP
 * by Jan-Willem Smaal <usenet@gispen.org>
 */
#include "midi1.h"

/*
 * Functions for the MIDI software based clock timer.
 */
//#include "midi1_clock_timer.h"

/*
 * Adjustable MIDI clock we feed it with the PLL adjustments.
 */
#include "midi1_clock_adj.h"

/*
 * Functions for measuring incoming MIDI clock signals
 * TODO: Note that #include "midi1_clock_measure_counter.h"  is
 * TODO: not working! don't use
 * TODO: this one.  Use only "midi1_clock_measure.h"
 */
#include "midi1_clock_measure.h"

/*
 * A Phase Locked Loop for MIDI.
 */
#include "midi1_clock_pll.h"

/* Provide the received 24pqn MIDI clock on a pin */
#define RX_MIDI_CLOCK_ON_PIN 1
#if RX_MIDI_CLOCK_ON_PIN
#define RX_MIDI_CLK DT_NODELABEL(rx_midi_clk)
static const struct gpio_dt_spec rx_midi_clk_pin = GPIO_DT_SPEC_GET(RX_MIDI_CLK, gpios);

static void main_rx_midi_clk_gpio_init(void)
{
	int ret = gpio_pin_configure_dt(&rx_midi_clk_pin, GPIO_OUTPUT_INACTIVE);
	if (ret < 0){
		printk("Error configing pin\n");
		return;
	}
}
#endif

/*
 * -- == Device Tree stuff == --
 */
#define USB_MIDI_DT_NODE DT_NODELABEL(usb_midi)
static const struct device *const midi = DEVICE_DT_GET(USB_MIDI_DT_NODE);

/* LED's */
static struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* We want logging */
LOG_MODULE_REGISTER(sample_usb_midi, LOG_LEVEL_INF);

static void key_press(struct input_event *evt, void *user_data)
{
	/* Only handle key presses in the 7bit MIDI range */
	if (evt->type != INPUT_EV_KEY || evt->code > 0x7f) {
		return;
	}
	uint8_t command = evt->value ? UMP_MIDI_NOTE_ON : UMP_MIDI_NOTE_OFF;
	uint8_t channel = 0;
	uint8_t note = evt->code;
	uint8_t velocity = 100;

	struct midi_ump ump = UMP_MIDI1_CHANNEL_VOICE(0, command, channel,
						      note, velocity);
	usbd_midi_send(midi, ump);
}

INPUT_CALLBACK_DEFINE(NULL, key_press, NULL);

static const struct ump_endpoint_dt_spec ump_ep_dt =
UMP_ENDPOINT_DT_SPEC_GET(USB_MIDI_DT_NODE);

const struct ump_stream_responder_cfg responder_cfg =
UMP_STREAM_RESPONDER(midi, usbd_midi_send, &ump_ep_dt);

static void on_midi_packet(const struct device *dev, const struct midi_ump ump)
{
	LOG_INF("Received MIDI packet (MT=%X)", UMP_MT(ump));

	switch (UMP_MT(ump)) {
	case UMP_MT_MIDI1_CHANNEL_VOICE:
		/* Only send MIDI1 channel voice messages back to the host */
		LOG_INF("Send back MIDI1 message %02X %02X %02X",
			UMP_MIDI_STATUS(ump), UMP_MIDI1_P1(ump),
			UMP_MIDI1_P2(ump));
		printk("Send back MIDI1 message %02X %02X %02X\n",
		       UMP_MIDI_STATUS(ump), UMP_MIDI1_P1(ump),
		       UMP_MIDI1_P2(ump));
		usbd_midi_send(dev, ump);
		break;
	case UMP_MT_UMP_STREAM:
		ump_stream_respond(&responder_cfg, ump);
		break;
	case UMP_MT_SYS_RT_COMMON:
		uint8_t status = UMP_MIDI_STATUS(ump);
		switch (status) {
		case RT_TIMING_CLOCK:	/* MIDI Clock */
#if RX_MIDI_CLOCK_ON_PIN
			/*
			 * toggle a PIN so we can measure
			 * on the scope the incoming clock.
			 */
			gpio_pin_toggle_dt(&rx_midi_clk_pin);
#endif
			midi1_clock_meas_pulse();
			uint32_t now_us = midi1_clock_meas_get_us();
			midi1_pll_process_tick(now_us);
			
#if 0
			/* Update generator right away */
			uint32_t interval_us = midi1_pll_get_interval_us();
			midi1_clock_adj_set_interval_us(interval_us);
			//midi1_pll_process_tick(midi1_clock_meas_last_timestamp());
#endif
			break;
		case RT_START:	/* Start */
			break;
		case RT_CONTINUE:	/* Continue */
			/* optional: resume measurement */
			break;
		case RT_STOP:	/* Stop */
			break;
		default:
			break;
		}
		break;
	default:
		printk("Unimplemented message %02X %02X %02X\n",
		       UMP_MIDI_STATUS(ump), UMP_MIDI1_P1(ump),
		       UMP_MIDI1_P2(ump));
		break;
	}

}

static void on_device_ready(const struct device *dev, const bool ready)
{
	/* Light up the LED (if any) when USB-MIDI2.0 is enabled */
	if (led0.port) {
		gpio_pin_set_dt(&led0, ready);
		k_msleep(100);
		gpio_pin_toggle_dt(&led0);
		k_msleep(100);
		gpio_pin_toggle_dt(&led0);
		k_msleep(100);
		gpio_pin_toggle_dt(&led0);
	}
}

static const struct usbd_midi_ops ops = {
	.rx_packet_cb = on_midi_packet,
	.ready_cb = on_device_ready,
};

/*
 * Init all the USB MIDI stuff in main.
 */
int main_midi_init()
{
	struct usbd_context *sample_usbd;

	if (!device_is_ready(midi)) {
		LOG_ERR("MIDI device not ready");
		return -1;
	}
	if (led0.port && led2.port) {
		if (gpio_pin_configure_dt(&led0, GPIO_OUTPUT)) {
			LOG_ERR("Unable to setup LED0, not using it");
			memset(&led0, 0, sizeof(led0));
		}
		if (gpio_pin_configure_dt(&led2, GPIO_OUTPUT)) {
			LOG_ERR("Unable to setup LED2, not using it");
			memset(&led2, 0, sizeof(led2));
		}
	}

	usbd_midi_set_ops(midi, &ops);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -1;
	}
	if (usbd_enable(sample_usbd)) {
		LOG_ERR("Failed to enable device support");
		return -1;
	}
	LOG_INF("USB device support enabled");
	
	/* Init the clock measurement system */
	// midi1_clock_init(midi);
	midi1_clock_adj_init(midi);
	midi1_clock_meas_init();
	/* We init the PLL with something and adjust from there */
	midi1_pll_init(12000);
	
	return 0;
}

/*-------------------- THREADS ------------------ */

/*
 * This blinks LED2 (blue) in the interval received via MIDI on every
 * quater note.
 */
void led_blink_thread(void)
{
	if (!device_is_ready(led2.port)) {
		printk("LED device not ready\n");
		return;
	}
	
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
	gpio_pin_toggle_dt(&led2);
	
	while (1) {
		/* Get current PLL tick interval (1/24 QN) */
		int32_t tick_us = midi1_clock_meas_last_interval();
		
		/* Convert to quarter-note interval */
		int32_t qn_us = abs(tick_us * 24u);
		
		/* Toggle LED */
		gpio_pin_toggle_dt(&led2);
		
		/* if the qn_us makes somewhat sense */
		if(qn_us < 2500000) {
			/* Sleep for 1/2  quarter note */
			k_usleep(qn_us/2);
		}
		else {
			/*
			 * if we get strange large values for
			 * qn_us > 2.5 seconds (1 BPM)
			 */
			k_msleep(100);
			printk("led_blink_thread: Large value qn_us: %d\n", qn_us);
			continue;
		}
	}
}

K_THREAD_DEFINE(led_blink_tid, 1024,
		led_blink_thread, NULL, NULL, NULL,
		5, 0, 0);

/**
 * Main thread - this may actually terminate normally (code 0) in zephyr.
 * and the rest of threads keeps running just fine.
 */
int main(void)
{
	/* Init the USB MIDI */
	if (main_midi_init()) {
		printk("Failed to main_midi_init()\n");
		return -1;
	}
#if RX_MIDI_CLOCK_ON_PIN
	main_rx_midi_clk_gpio_init();
#endif
	printk("main: MIDI ready entering main() loop\n");
	printk("main: Generate MIDI at 120.00 BPM (for 30s) \n");
	//midi1_clock_start_sbpm(12000);
	midi1_clock_adj_start_sbpm(12000);
	k_msleep(30000);
	while (1) {
		uint16_t raw_sbpm = midi1_clock_meas_get_sbpm();
		printk("main:     BPM (raw): %s\n", sbpm_to_str(raw_sbpm));
		uint16_t pll_sbpm = pqn24_to_sbpm(midi1_pll_get_interval_us());
		printk("main: PLL BPM      : %s\n", sbpm_to_str(pll_sbpm));
		/* Now adjust our own generated BPM... */
		uint32_t pll_int_pqn = midi1_pll_get_interval_us();
		printk("main: PLL int [us] : %u\n", pll_int_pqn);
		/* Adjust generated clock */
		midi1_clock_adj_set_interval_us(pll_int_pqn);
		uint16_t gen_sbpm = midi1_clock_adj_get_sbpm();
		printk("main: GEN BPM      : %s\n", sbpm_to_str(gen_sbpm));
		k_msleep(10000);
	}
	return 0;
}

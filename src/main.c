/**
 * MIDI 1.0 into Universal MIDI Packet over USB by J-W Smaal
 * using a sensor value's to send MIDI1.0 encapsulated into UMP
 * over USB.
 * @author Jan-Willem Smaal <usenet@gispen.org>
 *
 *
 * Adapted Original: Sample application for USB MIDI 2.0 device class
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

/*
 * This is part of the MIDI2 library prj.conf
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y /zephyr/lib/midi2/ump_stream_responder.h
 * it get's linked in.
 */
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <ump_stream_responder.h>

/**
 * Functions for MIDI1 encapulation into UMP
 * by Jan-Willem Smaal <usenet@gispen.org>
 */
#include "midi1.h"

/**
 * Functions for the MIDI clock timer.
 */
//#include "midi1_clock_timer.h"

/**
 * Functions for the MIDI clock timer.
 */
#include "midi1_clock_counter.h"


/**
 * Functions for measuring incoming MIDI clock
 */
#include "midi1_clock_measure.h"

/**
 * -- == Device Tree stuff == --
 */
#define USB_MIDI_DT_NODE DT_NODELABEL(usb_midi)
static const struct device *const midi = DEVICE_DT_GET(USB_MIDI_DT_NODE);

/* LED's */
static struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
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
			case RT_TIMING_CLOCK:   /* MIDI Clock */
				midi1_clock_meas_pulse();
				break;
			case RT_START:   	/* Start */
				midi1_clock_meas_init();
				break;
			case RT_CONTINUE:   	/* Continue */
				/* optional: resume measurement */
				break;
			case RT_STOP:   	/* Stop */
				midi1_clock_meas_init();
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
#if 0 
	/* Light up the LED (if any) when USB-MIDI2.0 is enabled */
	if (led0.port) {
		gpio_pin_set_dt(&led0, ready);
		k_msleep(700);
		gpio_pin_toggle_dt(&led0);
	}
#endif 
}

static const struct usbd_midi_ops ops = {
	.rx_packet_cb = on_midi_packet,
	.ready_cb = on_device_ready,
};

/**
 * Init all the USB MIDI stuff in main.
 */
int main_midi_init()
{
	struct usbd_context *sample_usbd;

	if (!device_is_ready(midi)) {
		LOG_ERR("MIDI device not ready");
		return -1;
	}
	if (led0.port && led1.port && led2.port) {
//		if (gpio_pin_configure_dt(&led0, GPIO_OUTPUT)) {
//			LOG_ERR("Unable to setup LED0, not using it");
//			memset(&led0, 0, sizeof(led0));
//		}
		if (gpio_pin_configure_dt(&led1, GPIO_OUTPUT)) {
			LOG_ERR("Unable to setup LED1, not using it");
			memset(&led1, 0, sizeof(led1));
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
	return 0;
}

/**
 * Main thread - this may actually terminate normally (code 0) in zephyr.
 * and the rest of threads keeps running just fine.
 */
int main(void)
{
	uint8_t channel = 7; /* This is MIDI Channel 8 ! */
	uint8_t controller = 1;
	uint8_t value = 63;
	uint8_t val = 32;
	uint8_t key = 100;
	uint8_t velocity = 100;
	int ret;

	/* Init the USB MIDI */
	if (main_midi_init()) {
		LOG_ERR("Failed to main_midi_init()");
		return -1;
	}
	LOG_INF("main: MIDI ready entering main() loop");

	midi1_clock_cntr_init(midi);

	while (1) {
		midi1_clock_cntr_stop();
		midi1_clock_cntr_init(midi);
		/* Test tempo changes  */
		for (int i = 4000u; i < 19000; i = i + 1000) {
			midi1_clock_meas_init();
			/* Hardware timer implementation */
			printk("BPM is: %d clock_freq: %d\n",
			       i,
			       midi1_clock_cntr_cpu_frequency());
			midi1_clock_cntr_ticks_start(
				sbpm_to_ticks( i,
					      midi1_clock_cntr_cpu_frequency()
					      )
				);
			k_msleep(7000);
			controller = 1;
			val = 0;
			for (int i = 0; i < 127; i++) {
				usbd_midi_send(midi,
					       midi1_controlchange(channel,
								   controller,
								   val));
				val++;
				k_msleep(100);
				/* Don't print this too often */
				if(!(i%10)) {
					printk("Measured BPM during: %u\n",
					       midi1_clock_meas_get_sbpm());
				}
			}
		}
	}
	return 0;
}

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
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h it get's linked in.
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
#include "midi1_clock_timer.h"


/**
 * -- == Device Tree stuff == --
 */
#define USB_MIDI_DT_NODE DT_NODELABEL(usb_midi)
static const struct device *const midi = DEVICE_DT_GET(USB_MIDI_DT_NODE);
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0),gpios,{0});

/* We want logging */
LOG_MODULE_REGISTER(sample_usb_midi, LOG_LEVEL_INF);



static void
key_press(struct input_event *evt, void *user_data)
{
	/* Only handle key presses in the 7bit MIDI range */
	if (evt->type != INPUT_EV_KEY || evt->code > 0x7f) {
		return;
	}
	uint8_t		command = evt->value ? UMP_MIDI_NOTE_ON : UMP_MIDI_NOTE_OFF;
	uint8_t		channel = 0;
	uint8_t		note = evt->code;
	uint8_t		velocity = 100;

	struct midi_ump	ump = UMP_MIDI1_CHANNEL_VOICE(0, command, channel,
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
		UMP_MIDI_STATUS(ump), UMP_MIDI1_P1(ump), UMP_MIDI1_P2(ump));
		usbd_midi_send(dev, ump);
		break;
	case UMP_MT_UMP_STREAM:
		ump_stream_respond(&responder_cfg, ump);
		break;
	}
}


static void on_device_ready(const struct device *dev, const bool ready)
{
	/* Light up the LED (if any) when USB-MIDI2.0 is enabled */
	if (led.port) {
		gpio_pin_set_dt(&led, ready);
	}
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

	if (led.port) {
		if (gpio_pin_configure_dt(&led, GPIO_OUTPUT)) {
			LOG_ERR("Unable to setup LED, not using it");
			memset(&led, 0, sizeof(led));
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
		uint8_t channel = 7;
		uint8_t controller = 1;
	uint8_t		value = 63;
	uint8_t		val = 32;
	uint8_t		key = 100;
	uint8_t		velocity = 100;
	int		ret;

	/* Init the USB MIDI */
	if (main_midi_init()) {
		LOG_ERR("Failed to main_midi_init()");
		return -1;
	}

	LOG_INF("main: MIDI ready entering main() loop");

	midi_clock_init(midi);
    // midi_clock_stop();
	
	while (1) {
		/* Test tempo  */ 
		for (int i = 8000u; i < 14000; i = i + 1000) {
			midi_clock_stop();
			midi_clock_init(midi);
			midi_clock_start(sbpm_to_24pqn(i));
			k_msleep(10000); 
		}

		controller = 1;
		val = 0;

		for (int i = 0; i < 127; i++) {
			/* usbd_midi_send(midi, midi1_timing_clock()); */
			usbd_midi_send(midi, midi1_controlchange(
									channel,
									controller,
									val));
			val++;
			k_msleep(100);
		}
	}

	return 0;
}

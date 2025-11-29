/**
 * MIDI test by J-W Smaal
 *
 * Adapted Original:
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample application for USB MIDI 2.0 device class
 */
#include <sample_usbd.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/usb/class/usbd_midi2.h>

#include <ump_stream_responder.h>
#include <zephyr/logging/log.h>

#include "midi1.h"

LOG_MODULE_REGISTER(sample_usb_midi, LOG_LEVEL_INF);
#define USB_MIDI_DT_NODE DT_NODELABEL(usb_midi)
static const struct device *const midi = DEVICE_DT_GET(USB_MIDI_DT_NODE);
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});


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

int main(void)
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


	
	/* Example: CC1 (Mod Wheel) value = 64 on channel 1 */
	uint8_t group      = 0;   /* UMP group (usually 0 unless multi‑group setup) */
	uint8_t command    = UMP_MIDI_CONTROL_CHANGE;
	uint8_t channel    = 0;   /* Channel 1 → zero‑based index 0 */
	uint8_t controller = 1;   /* CC number 1 = Modulation Wheel */
	uint8_t value      = 64;  /* CC value (0–127) */
	
	struct midi_ump ump1 = UMP_MIDI1_CHANNEL_VOICE(group,
												  command,
												  channel,
												  controller,
												  value);
	struct midi_ump ump2;
	struct midi_ump ump3;
	


	/* Start a timer that sends a control change message */ 
	while (1) {
		//usbd_midi_send(dev, ump);
		LOG_INF("main: sleeping for 1 second");
		k_msleep(1000);
		
		ump1 = UMP_MIDI1_CHANNEL_VOICE(group,
									  command,
									  channel,
									  controller,
									  value);
		
		ump2 = Midi1ControlChange(channel, controller, value);
		ump3 = Midi1ModWheel(channel, value);
		
		/* Send it over USB-MIDI */
		usbd_midi_send(midi, ump1);
		usbd_midi_send(midi, ump2);
		usbd_midi_send(midi, ump2);
		if (value >= 127) {
			value = 0;
		} else {
			value = value + 1;
		}
	}

	return 0;
}

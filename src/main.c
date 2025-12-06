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
 * Sensor functions
 */
#include <zephyr/drivers/sensor.h>


/*
 * This is part of the MIDI2 library
 * prj.conf CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h
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
 * Functions for the accelator/magnetometer sensor
 */
#include "orientation.h"

/**
 * -- == Device Tree stuff == --
 */
#define USB_MIDI_DT_NODE DT_NODELABEL(usb_midi)
static const struct device *const midi = DEVICE_DT_GET(USB_MIDI_DT_NODE);
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0),
													 gpios,
													 {0});

/* We want logging */
LOG_MODULE_REGISTER(sample_usb_midi, LOG_LEVEL_INF);



/* Helper: scale float angle (degrees) into 7-bit MIDI CC value (0–127) */
static inline uint8_t angle_to_cc(float angle, float min_deg, float max_deg)
{
	/* Clamp */
	if (angle < min_deg) angle = min_deg;
	if (angle > max_deg) angle = max_deg;
	
	/* Normalize to 0–127 */
	float norm = (angle - min_deg) / (max_deg - min_deg);
	int scaled = (int)(norm * 127.0f);
	
	if (scaled < 0) scaled = 0;
	if (scaled > 127) scaled = 127;
	
	return (uint8_t)scaled;
}

void send_orientation_cc(const struct device *midi,
						 uint8_t channel,
						 struct orientation_angles angles)
{
	/* Map pitch, roll, heading to different CC numbers */
	uint8_t pitch_val   = angle_to_cc(angles.pitch,  -90.0f,  90.0f);   // CC#2 Breath
	uint8_t roll_val    = angle_to_cc(angles.roll, -180.0f, 180.0f);   // CC#3 Undefined
	uint8_t heading_val = angle_to_cc(angles.heading,   0.0f, 360.0f); // CC#1 Mod Wheel
	
	/* Send CC messages */
	usbd_midi_send(midi, Midi1ControlChange(channel,
											CTL_MSB_MAIN_VOLUME,
											pitch_val));
	usbd_midi_send(midi, Midi1ControlChange(channel,
											CTL_MSB_BALANCE,
											roll_val));
	usbd_midi_send(midi, Midi1ControlChange(channel,
											CTL_MSB_MODWHEEL,
											heading_val));
}

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


/**
 * Init all the USB MIDI stuff in main.
 */
int main_midi_init(){
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
	//uint8_t group      = 0;   // UMP group (usually 0 unless multi‑group setup)
	uint8_t channel    = 7;   // Channel 8 → zero‑based index 0
	uint8_t controller = 2;
	uint8_t value      = 63;
	uint8_t val		   = 32;
	uint8_t key		   = 100;
	uint8_t velocity   = 100;
	struct sensor_value mag[3];
	struct sensor_value accel[3];
	struct orientation_angles angles;
	int ret;


	/* Init the USB MIDI */
	if (main_midi_init()) {
		LOG_ERR("Failed to main_midi_init()");
		return -1;
	}
	
	/*
	 * Init sensor stuff
	 */
	const struct device *const dev = DEVICE_DT_GET_ONE(nxp_fxos8700);
	
	if (!device_is_ready(dev)) {
		LOG_ERR("sensor: device not ready");
		return -1;
	}
	LOG_INF("main: sensor and MIDI ready entering main() loop");
	
	while (1) {
		k_msleep(80);
		
		/* Read some sensor */
		ret = sensor_sample_fetch(dev);
		if (ret < 0) {
			LOG_ERR("sensor_sample_fetch failed ret %d\n", ret);
			return -1;
		}
		
		/* read acceleration channels*/
		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel[0]);
		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel[1]);
		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel[2]);
		
		/* Read magnetic field channels */
		sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &mag[0]);
		sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &mag[1]);
		sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &mag[2]);
		
		/* Print values in microtesla (µT) */
		//LOG_INF("Magnetometer: X=%d.%06d µT, Y=%d.%06d µT, Z=%d.%06d µT\n",
		//		mag[0].val1, mag[0].val2,
		//		mag[1].val1, mag[1].val2,
		//		mag[2].val1, mag[2].val2);
		/* Compute orientation */
		angles = orientation_compute(accel, mag);
		
		printf("Pitch=%.2f°, Roll=%.2f°, Heading=%.2f°\n",
			   (double)angles.pitch,
			   (double)angles.roll,
			   (double)angles.heading);
#if 0
		LOG_INF("LOG Pitch=%d.%02d°, Roll=%d.%02d°, Heading=%d.%02d°",
				(int)angles.pitch,
				abs((int)(angles.pitch * 100) % 100),
				(int)angles.roll,
				abs((int)(angles.roll * 100) % 100),
				(int)angles.heading,
				abs((int)(angles.heading * 100) % 100));
#endif
		
		/* Send it over USB-MIDI */
		//usbd_midi_send(midi, UMP_MIDI1_CHANNEL_VOICE(group,
		//											 command,
		//											 channel,
		//											 controller,
		//											 value));
		//usbd_midi_send(midi, Midi1ControlChange(channel,
		//										controller,
		//										value));
		//usbd_midi_send(midi, Midi1ModWheel(channel,
		//								   value));
		//usbd_midi_send(midi, Midi1PitchWheel(channel,
		//									 value));
		
		/* Send orientation mapped to MIDI CC */
		send_orientation_cc(midi, 12, angles);
#if MIDI_NOTES
		usbd_midi_send(midi,
					   Midi1NoteON(channel,
									key,
									velocity));

		/* Leave some time in between the note on and off to test */
		k_msleep(100);

		usbd_midi_send(midi,
					   Midi1NoteOFF(channel,
									key,
									velocity));
#endif
		
		/* Test sending all control change messages */
#if CONTROL_CHANGE_TEST
		controller = 0;
		for (int i = 0; i < 127; i++) {
			usbd_midi_send(midi,
						   Midi1ControlChange(channel,
											  controller,
											  val));
			controller++;
			k_msleep(10);
		}
#endif
		//usbd_midi_send(midi, Midi1PitchWheel(channel,
		//									 val));
		//usbd_midi_send(midi, Midi1ModWheel(channel,
		//								   val));
		//usbd_midi_send(midi, Midi1PolyAfterTouch(channel,
		//										 key,
		//										 val));
		//usbd_midi_send(midi, Midi1ChannelAfterTouch(channel,
		//											val));

		if (value >= 127) {
			value = 0;
			val = 0;
			key = 0;
			velocity = 0;
			controller = 0;	/* it's actually MSB select */
		} else {
			value++;
			val++;
			key++;
			velocity++;
			controller++;
		}
	}

	return 0;
}

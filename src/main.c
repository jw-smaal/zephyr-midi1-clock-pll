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
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
//#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

/* ------------------------------------------------ */
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/sys/printk.h>
/* ------------------------------------------------ */

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
 * MIDI1.0 serial 5 port DIN port support
 */
#include "midi1_serial.h"

/*
 * Functions for the MIDI software based clock timer.
 */
//#include "midi1_clock_timer.h"

/*
 * Functions for the MIDI PIT0_CHANNEL0 hardware based clock timer.
 */
#include "midi1_clock_counter.h"

/*
 * Adjustable MIDI clock we feed it with the PLL adjustments.
 * TODO: external measurements show this clock is too slow...
 * TODO: maybe due to the adjustable scheduled work timer
 */
//#include "midi1_clock_adj.h"

/*
 * Functions for measuring incoming MIDI clock signals
 */
//#include "midi1_clock_measure.h"
#include "midi1_clock_measure_counter.h"

/*
 * A Phase Locked Loop for MIDI.
 */
//#include "midi1_clock_pll.h"
#include "midi1_clock_pll_ticks.h"


/* ------------------------------------------------------------------------- */

/* Provide the received 24pqn MIDI clock on a pin */
#define RX_MIDI_CLOCK_ON_PIN 1
#if RX_MIDI_CLOCK_ON_PIN
#define RX_MIDI_CLK DT_NODELABEL(rx_midi_clk)
static const struct gpio_dt_spec rx_midi_clk_pin =
GPIO_DT_SPEC_GET(RX_MIDI_CLK, gpios);

static void main_rx_midi_clk_gpio_init(void)
{
	int ret = gpio_pin_configure_dt(&rx_midi_clk_pin, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Error configing pin\n");
		return;
	}
}
#endif


#define USB_MIDI_DT_NODE DT_NODELABEL(usb_midi)
static const struct device *const midi = DEVICE_DT_GET(USB_MIDI_DT_NODE);

/* LED's */
static struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* We want logging */
LOG_MODULE_REGISTER(sample_usb_midi, LOG_LEVEL_INF);



/* ----------------------- handlers callbacks  ----------------------------- */
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

/* TODO: work in progress handler for timing purposes */
static void on_ump_packet(const struct device *dev, const struct midi_ump ump)
{
	switch (UMP_MT(ump)) {
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
			midi1_clock_meas_cntr_pulse();
			midi1_pll_ticks_process_interval
			    (midi1_clock_meas_cntr_interval_ticks());
			break;
		default:
			break;
		}
	default:
		break;
	}
}

#if 0
/* handler for all */
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
			//midi1_clock_meas_pulse();
			midi1_clock_meas_cntr_pulse();
			//uint32_t now_us = midi1_clock_meas_get_us();
			//midi1_pll_process_tick(now_us);

			/* Update generator right away */
			uint32_t interval_us = midi1_pll_get_interval_us();

			/* Reschedule job as well */
			midi1_clock_adj_set_interval_us(interval_us);
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
#endif

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

/* different rx callback for the clock tests */
static const struct usbd_midi_ops ump_ops = {
	.rx_packet_cb = on_ump_packet,
	.ready_cb = on_device_ready,
};

/*
static const struct usbd_midi_ops ops = {
	.rx_packet_cb = on_midi_packet,
	.ready_cb = on_device_ready,
};
*/



/**
 * @brief Callbacks/delegates for 'midi1_serial.c' after parsing MIDI1.0
 *
 * @note
 * Do not block in these function as they are called from the MIDI
 * parser this one is blocked untill the delegate is finished.
 */
void note_on_handler(uint8_t note, uint8_t velocity) {
	printk("Note  on: %03d %03d\n", note, velocity);
}

void note_off_handler(uint8_t note, uint8_t velocity) {
	printk("Note off: %03d %03d\n", note, velocity);
}

void midi_pitchwheel_handler(uint8_t lsb, uint8_t msb) {
	/* 14 bit value for the pitch wheel  */
	int16_t pwheel = (int16_t)((msb << 7) | lsb) - PITCHWHEEL_CENTER ;
	
	/* print on the serial out */
	printk("Pitchwheel: %d\n", pwheel);
}

void control_change_handler_model(uint8_t controller, uint8_t value) {
	printk("Control change: %d %d\n", controller, value);
}

void control_change_handler(uint8_t controller, uint8_t value) {
	printk("Control change: %d %d\n", controller, value);
}

void realtime_handler(uint8_t msg) {
	printk("Realtime: %d\n", msg);
}


/* ------------------------- INIT functions -------------------------------- */
/*
 * Init all the USB MIDI stuff in main.
 */
int main_midi_init()
{
	struct usbd_context *sample_usbd;

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
#if RX_MIDI_CLOCK_ON_PIN
	main_rx_midi_clk_gpio_init();
#endif

	if (!device_is_ready(midi)) {
		LOG_ERR("MIDI device not ready");
		return -1;
	}
	//usbd_midi_set_ops(midi, &ump_ops);
	// For timing tests
	usbd_midi_set_ops(midi, &ump_ops);
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
	midi1_clock_cntr_init(midi);
	midi1_clock_meas_cntr_init();
	
	/* We init the PLL with something and adjust from there */
	midi1_pll_ticks_init(12000);
	
	/* defined in midi1_serial.h */
	/* Initialize the MIDI parser with the callbacks */
	SerialMidiInit(&note_on_handler,
		       &note_off_handler,
		       &control_change_handler,
		       &realtime_handler,
		       &midi_pitchwheel_handler);
	printk("MIDI1.0 serial initialized\n");
	
	/*
	 * Send example MIDI messages to test the DIN5 MIDI1.0
	 */
#define TEST_MIDI_OUTPUT 0
#if TEST_MIDI_OUTPUT
	for (int j =0 ; j < 16; j++ ) {
		for (int i = 0; i < 16; i++) {
			printk("MIDI1.0 serial NoteON\n");
			SerialMidiNoteON(j,60,i);
			k_msleep(100);
		}
		for (int i = 0; i < 16; i++) {
			printk("MIDI1.0 serial NoteON (velocity=0)\n");
			SerialMidiNoteON(j,60,0);
			k_msleep(100);
		}
		k_msleep(2000);
	}
#endif
	return 0;
}



/* Get the display device (DTS node must be named sh1106) */
const struct device *display = DEVICE_DT_GET(DT_NODELABEL(sh1106));
/* in the root of the device tree we need to point it to the sh1106 */ 
const struct device *cfb = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

int main_display_init(void)
{
	printk("SH1106 display init \n");

	if (!device_is_ready(display)) {
		printk("Display not ready\n");
		return -1;
	}

	if (!device_is_ready(cfb)) {
		printk("CFB not ready\n");
		return -1;
	}

	/* Turn on the panel */
	display_blanking_off(display);

	/* Initialize the character framebuffer */
	if (cfb_framebuffer_init(cfb)) {
		printk("CFB init failed\n");
		return -1;
	}

	/* Clear the framebuffer */
	cfb_framebuffer_clear(cfb, true);

	/* Select font index 0 (usually 8x8) */
	cfb_framebuffer_set_font(cfb, 0);

	/* Print default text */
	cfb_print(cfb, "MIDIsync JWS", 0, 0);
	cfb_print(cfb, "xxx.xx BPM  ", 0, 16);
	cfb_print(cfb, "  -=init=-  ", 0, 32);

	/* Push framebuffer to display */
	cfb_framebuffer_finalize(cfb);
	return 0; 
}





/* ---------------------------- THREADS ------------------------------------ */

/*
 * MIDI1.0 5PIN DIN serial receive parser thread. 
 */
void midi1_serial_receive_thread(void) {
	while (1) {
		/* This one is blocking now */
		SerialMidiReceiveParser();
	}
}
K_THREAD_DEFINE(midi1_serial_receive_tid, 512,
		midi1_serial_receive_thread, NULL, NULL, NULL, 5, 0, 0);


/**
 * @brief helper function to print the scaled BPM to the display
 * @param sbpm scaled bpm parameter
 * TODO: check return values
 */
void display_update_bpm_line(uint16_t sbpm) {
	/* printk("display_update_bpm: %s\n", sbpm_to_str(sbpm)); */
	cfb_print(cfb, sbpm_to_str(sbpm), 0, 16);
	cfb_framebuffer_finalize(cfb);
	return;
}

/*
 * This blinks LED2 (blue) in the interval received via MIDI-USB on
 * every quater note. It also shows a spinner in the display and updates
 * the measured BPM.
 */
void led_blink_thread(void)
{
	if (!device_is_ready(led2.port)) {
		printk("LED device not ready\n");
		return;
	}

	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
	gpio_pin_toggle_dt(&led2);
	int spinner = 0;
	
	while (1) {
		/* Get current PLL tick interval (1/24 QN) */
		int32_t tick_us = midi1_clock_meas_cntr_interval_us();

		/* Convert to quarter-note interval so multiply by 24 */
		int32_t qn_us = abs(tick_us * 24u);

		/* Toggle LED */
		gpio_pin_toggle_dt(&led2);
				
		/* if the qn_us makes somewhat sense */
		if (qn_us < 2500000) {
			/* Update the BPM display */
			uint16_t cntr_sbpm = midi1_clock_meas_cntr_get_sbpm();
			/* Show it on the tiny display */
			display_update_bpm_line(cntr_sbpm);
			
			/* In the sync with the clock rotate the spinner */
			switch(spinner ) {
				case 0:
					cfb_print(cfb, "/ (synced)", 0, 32);
					cfb_framebuffer_finalize(cfb);
					spinner++;
					break;
				case 1:
					cfb_print(cfb, "- (synced)", 0, 32);
					cfb_framebuffer_finalize(cfb);
					spinner++;
					break;
				case 2:
					cfb_print(cfb, "\\ (synced)", 0, 32);
					cfb_framebuffer_finalize(cfb);
					spinner++;
					break;
				case 3:
					cfb_print(cfb, "| (synced)", 0, 32);
					cfb_framebuffer_finalize(cfb);
					spinner = 0;
					break;
				default:
					spinner = 0;
					break;
			}
			/* Sleep for 1/2  quarter note */
			k_usleep(qn_us / 2);
		} else {
			/*
			 * if we get strange large values for
			 * qn_us > 2.5 seconds (1 BPM) just ignore it.
			 */
			k_msleep(2000);
			
			/* printk("led_blink_thread: Large value qn_us: %d\n",
			    qn_us); */
			continue;
		}
		/* Update the display */ 
	}
}
K_THREAD_DEFINE(led_blink_tid, 512,
		led_blink_thread, NULL, NULL, NULL, 5, 0, 0);

#include "banner.h"
/**
 * Main thread - this may actually terminate normally (code 0) in zephyr.
 * and the rest of threads keeps running just fine.
 */
int main(void)
{
	/* Serial boot screen */ 
	printk("%s", banner);

	/* Start the display stuff */
	//main_display_init(); 
	//if(main_display_init()) {
	//	printk("Failed to main_display_init()\n");
	//}	

	/* Init the USB MIDI and the rest of the MIDI processes */
	if (main_midi_init()) {
		printk("Failed to main_midi_init()\n");
		return -1;
	}

	/* Sleep for 6 seconds so we can get some measurements intervals */
	k_msleep(6000);
	printk("--== Clock glitch testing by Jan-Willem Smaal v0.5 ==-- \n\n");
	printk("main: MIDI ready entering main() loop\n");
	printk("midi1_clock_cntr_get_sbpm: %s\n",
	       sbpm_to_str(midi1_clock_cntr_get_sbpm()));
	printk("midi1_clock_cntr_cpu_frequency: %u\n",
	       midi1_clock_cntr_cpu_frequency());
	
	/* Set the initial clock again because the PLL gets a init of 120 */
	uint32_t pll_ticks = midi1_pll_ticks_get_interval_ticks();
	midi1_clock_cntr_ticks_start(pll_ticks);
	
	/* Don't start the display straight after powerup (needs some time to settle */ 
	main_display_init(); 

	while (1) {
		/*  measure incoming interval. */
		printk("interval measured as: %u us\n",
		       midi1_clock_meas_cntr_interval_us());
		printk("interval measured as: %u ticks\n",
		       midi1_clock_meas_cntr_interval_ticks());
		
		uint16_t raw_cntr_sbpm = midi1_clock_meas_cntr_get_sbpm();
		printk("main cntr BPM (raw): %s\n", sbpm_to_str(raw_cntr_sbpm));
		
		/* Get pll ticks */
		uint32_t pll_ticks = midi1_pll_ticks_get_interval_ticks();
		printk("main: PLL ticks     : %d\n", pll_ticks);
		
		
		
		/* Half a minute of correct phase */
		for (int i = 0; i < 3; i++) {
			
			printk("main: -- in PHASE -- \n");
			/* Start the clock with the correct ticks */
			uint32_t pll_ticks = midi1_pll_ticks_get_interval_ticks();
			midi1_clock_cntr_ticks_start(pll_ticks);
			k_msleep(10000);
		}
#if 0
		/* shifting phase */
		for (int phases = 5000 ; phases <= 50000; phases += 2000) {
			printk("main: shifting phase: %u\n", phases);
			/* Start the clock with the phase shifted ticks */
			uint32_t pll_ticks = midi1_pll_ticks_get_interval_ticks();
			midi1_clock_cntr_ticks_start(pll_ticks - phases);
			k_msleep(5000);
		}
#endif
	}
 

	return 0;
}

/* EOF */

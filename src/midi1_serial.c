/**
 * @file midi1_serial.c
 * @brief MIDI USART implementation on Zephyr RTOS  
 *
 * @note 
 * MIDI USART implementation that also implements
 * MIDI "running status".
 *
 * Not many MIDI MCU example implementations (that i have seen) do this.
 * It's a very useful method to limit messages on
 * serial UART MIDI channels that are only 31250 Baud. It reduces
 * playing latency. It's essential when working with real gear.
 * 
 *  The MIDI USART implementation is for the Zephyr RTOS
 *  and uses the ring buffer and UART driver.
 *
 *  TODO:  - Use the return values from uart_send( and return them.
 *
 *  TODO:  - Extend callback functions to be channel aware (right now they are
 *  TODO:  - OMNI ALL --> i.e. all channels).
 *  TODO:  - should be easy to add as we already have running_status.
 *
 *  TODO:  - Error handling for the parser right now it's ignored.
 *  TODO:  - e.g. we could add DBG logging for this.
 *
 * Created in 2014 ported to Zephyr RTOS in 2024. 
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * updated 20241224
 * updated 20260103
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include "midi1.h"
#include "midi1_serial.h"

/* Private/hidden Prototype's for the ISR callback asssigned during init */
static void midi1_serial_isr_callback(const struct device *dev, void *user_data);

/**
 * Inits the serial USART with MIDI clock speed and
 */
int midi1_serial_init(struct midi1_serial_inst *inst)
{
	inst->running_status_rx = 0;
	inst->third_byte_flag = 0;
	inst->midi_c2 = 0;
	inst->midi_c3 = 0;

	inst->running_status_tx = 0;
	inst->running_status_tx_count = 0;
	
	/* If a null pointer is given reassign to the NO OP function */
	if (!inst->note_on) {
		inst->note_on = midi1_noop_note_on;
	}
	if (!inst->note_off) {
		inst->note_off = midi1_noop_note_off;
	}
	if (!inst->control_change) {
		inst->control_change = midi1_noop_control_change;
	}
	if (!inst->realtime) {
		inst->realtime = midi1_noop_realtime;
	}
	if (!inst->pitchwheel) {
		inst->pitchwheel = midi1_noop_pitchwheel;
	}
	if (!inst->program_change) {
		inst->program_change = midi1_noop_program_change;
	}
	if (!inst->channel_aftertouch) {
		inst->channel_aftertouch = midi1_noop_channel_aftertouch;
	}
	if (!inst->poly_aftertouch) {
		inst->poly_aftertouch = midi1_noop_poly_aftertouch;
	}

	/* Assign a MSQ to this instance */
	k_msgq_init(&inst->msgq, inst->msgq_buffer, MSG_SIZE, MSGQ_SIZE);
	
	if (!device_is_ready(inst->uart)) {
		printk("UART device not found!");
		return -1;
	}
	int ret =
	    uart_irq_callback_user_data_set(inst->uart,
					    midi1_serial_isr_callback,
					    inst);
	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk
			    ("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk
			    ("UART  does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return ret;
	}

	uart_irq_rx_enable(inst->uart);
	printk("midi1_serial_init() done");
	return 0;
}

/*
 * All functions related to sending MIDI messages to the serial USART
 */

/* Channel mode messages */
/* ___ _                       _   __  __         _
  / __| |_  __ _ _ _  _ _  ___| | |  \/  |___  __| |___
 | (__| ' \/ _` | ' \| ' \/ -_) | | |\/| / _ \/ _` / -_)
  \___|_||_\__,_|_||_|_||_\___|_| |_|  |_\___/\__,_\___|
 */

void midi1_serial_note_on(struct midi1_serial_inst *inst, uint8_t channel, uint8_t key, uint8_t velocity)
{
	 if ((C_NOTE_ON | channel) != inst->running_status_tx) {
                uart_poll_out(inst->uart, C_NOTE_ON | channel);
                inst->running_status_tx = C_NOTE_ON | channel;
        }
	uart_poll_out(inst->uart, key);
        uart_poll_out(inst->uart, velocity);
}

void midi1_serial_note_off(struct midi1_serial_inst *inst, uint8_t channel, uint8_t key, uint8_t velocity)
{
	if ((C_NOTE_OFF | channel) != inst->running_status_tx) {
		uart_poll_out(inst->uart, C_NOTE_OFF | channel);
		inst->running_status_tx = C_NOTE_OFF | channel;
	}
	uart_poll_out(inst->uart, key);
	uart_poll_out(inst->uart, velocity);
}

/*
 * Even though we keep running status on TX we retransmit every 16'th
 * time to make sure the receiver is in sync even when some messages
 * are lost. MIDI recommendations are also if no message was sent in the last
 * 300ms to also include the status byte.
 * Running status is most important for smooth control changes.
 * TODO: implement this check also for the other functions.
 */
void midi1_serial_control_change(struct midi1_serial_inst *inst,
				 uint8_t channel,
				 uint8_t controller,
				 uint8_t val)
{

	if (inst->running_status_tx_count >= 16) {
		inst->running_status_tx_count = 0;
		uart_poll_out(inst->uart, C_CONTROL_CHANGE | channel);
		inst->running_status_tx = C_CONTROL_CHANGE | channel;
	}
	/* If we don't have running status send out the status byte. */
	else if ((C_CONTROL_CHANGE | channel) != inst->running_status_tx) {
		uart_poll_out(inst->uart, C_CONTROL_CHANGE | channel);
		inst->running_status_tx = C_CONTROL_CHANGE | channel;
		inst->running_status_tx_count = 0;
	}
	/* We always send out controller and value */
	uart_poll_out(inst->uart, controller);
	uart_poll_out(inst->uart, val);
	inst->running_status_tx_count++;
}

void midi1_serial_channelaftertouch(struct midi1_serial_inst *inst,
				 uint8_t channel,
				 uint8_t val)
{
	if ((C_CHANNEL_AFTERTOUCH | channel) != inst->running_status_tx) {
		uart_poll_out(inst->uart, C_CHANNEL_AFTERTOUCH | channel);
		inst->running_status_tx = C_CHANNEL_AFTERTOUCH | channel;
	}
	uart_poll_out(inst->uart, val);
}

void midi1_serial_modwheel(struct midi1_serial_inst *inst,
			uint8_t channel,
			uint16_t val)
{
	midi1_serial_control_change(inst,
				    channel,
				    CTL_MSB_MODWHEEL,
				    ~(CHANNEL_VOICE_MASK) & (val >> 7));
	midi1_serial_control_change(inst,
				    channel,
				    CTL_LSB_MODWHEEL,
				    ~(CHANNEL_VOICE_MASK) & val);
}

void midi1_serial_pitchwheel(struct midi1_serial_inst *inst,
			     uint8_t channel,
			     uint16_t val)
{
	if (inst->running_status_tx != (C_PITCH_WHEEL | channel)) {
		uart_poll_out(inst->uart, C_PITCH_WHEEL | channel);
		inst->running_status_tx = C_PITCH_WHEEL | channel;
	}
	/* Value is 14 bits so need to shift 7 */
	uart_poll_out(inst->uart,
		      val & ~(CHANNEL_VOICE_MASK));	   /* LSB */
	uart_poll_out(inst->uart,
		      (val >> 7) & ~(CHANNEL_VOICE_MASK)); /* MSB */
	
}


/* System Common messages */
/*___         _                ___
 / __|_  _ __| |_ ___ _ __    / __|___ _ __  _ __  ___ _ _
 \__ \ || (_-<  _/ -_) '  \  | (__/ _ \ '  \| '  \/ _ \ ' \
 |___/\_, /__/\__\___|_|_|_|  \___\___/_|_|_|_|_|_\___/_||_|
 */
void midi1_serial_timingclock(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_TIMING_CLOCK);
}

void midi1_serial_start(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_START);
}

void midi1_serial_continue(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_CONTINUE);
}

void midi1_serial_stop(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_STOP);
}

void midi1_serial_active_sensing(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_ACTIVE_SENSING);
}

void midi1_serial_reset(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_RESET);
}


/*__  __ ___ ___ ___   ___      _         ___
 |  \/  |_ _|   \_ _| | _ )_  _| |_ ___  | _ \__ _ _ _ ___ ___ _ _
 | |\/| || || |) | |  | _ \ || |  _/ -_) |  _/ _` | '_(_-</ -_) '_|
 |_|  |_|___|___/___| |___/\_, |\__\___| |_| \__,_|_| /__/\___|_|
 |__/
 */

/**
 * @brief MIDI Byte Receive parser implementation - Interrupt Service Routine
 *
 * @param device device pointer
 * @param user_data user data (in our case the instance struct)
 * @note we use a Message Queue FIFO buffer to store the incoming MIDI messages
 * @note TODO: _NEW_ implementation needs testing
 */
static void midi1_serial_isr_callback(const struct device *dev, void *user_data)
{
	uint8_t c;
	/* Need to cast the user data to our instance struct */
	struct midi1_serial_inst *inst = (struct midi1_serial_inst *)user_data;
	
	if (!uart_irq_update(inst->uart)) {
		return;
	}
	
	if (!uart_irq_rx_ready(inst->uart)) {
		return;
	}
	
	/* read until FIFO empty */
	while (uart_fifo_read(inst->uart, &c, 1) == 1) {
		if (k_msgq_put(&inst->msgq, &c, K_NO_WAIT) != 0) {
			/* Message queue is full, TODO: maybe handle overflow */
		}
	}
}

/**
 * @brief Parse one byte at a time for the MIDI parsing.
 *
 * @note Then callback functions are called for each complete MIDI message.
 *
 * @note This function is really long as the MIDI parsing is done
 * @note byte per byte and it took several tests to get the parsing
 * @note done right.  I tried to create a statemachine earlier but
 * @note failed so I have sticked to my proven implementation.
 *
 * TODO: _NEW_ version
 * TODO: implement callbacks that include the MIDI channel as well!
 */
void midi1_serial_receiveparser(struct midi1_serial_inst *inst)
{
	uint8_t c;
	
	/*
	 * Read only _one_ byte from the circular FIFO input buffer
	 * This buffer is filled by the ISR routine on receipt of
	 * data on the port.
	 */
	if (k_msgq_get(&inst->msgq, &c, K_FOREVER) != 0) {
		return;
	} else {
		/* Valid message received ! */
		printk("%2X ", c);
	}
	
	/*
	 * Future implementation option
	 * To allow software MIDI THRU (kind of with some processing delay)
	 * simply write what is received to the output.
	 * uart_poll_out(midi, c);
	 */
	
	/* Check if bit7 = 1 */
	if (c & CHANNEL_VOICE_MASK) {
		/* if (! (c & SYSTEM_REALTIME_MASK)) */
		/*
		 * is it a real-time message?  0xF8 up to 0xFF
		 * decided not to create seperate callbacks for
		 * realtime messages.  This callback should check
		 * the 'enum midi_real_time' values and then
		 * decide if e.g. a clock is received or a RT_STOP etc..
		 */
		if (c >= 0xF8) {
			inst->realtime(c);
			return;
		} else {
			inst->running_status_rx = c;
			inst->third_byte_flag = 0;
			/* Is this a tune request */
			if (c == SYSTEM_TUNE_REQUEST) {
				inst->midi_c2 = c;	/*  Store in FIFO. */
				/* TODO: Process something. */
				return;
			}
			/*
			 * Do nothing
			 * Ignore for now
			 */
			return;
		}
	} else {		/* Bit 7 == 0   (data) */
		if (inst->third_byte_flag == 1) {
			inst->third_byte_flag = 0;
			inst->midi_c3 = c;
			
			/*
			 * TODO: work in progress add the 'chan' to the
			 * TODO: callbacks
			 */
			uint8_t common = inst->running_status_rx & SYSTEM_COMMON_MASK;
			uint8_t chan = inst->running_status_rx & ~(SYSTEM_COMMON_MASK);
			
			if (common == C_NOTE_ON) {
				if (inst->midi_c3 == 0) {
					/*
					 * Some MIDI implementations use
					 * velocity zero "note on"
					 * as a "note-off".  Other do use a
					 * note off and the note off velocity
					 * actually can be used to alter the
					 * timbre of the note off.
					 */
					inst->note_off(chan,
						       inst->midi_c2,
						       inst->midi_c3);
					return;
				} else {
					inst->note_on(chan,
						      inst->midi_c2,
						      inst->midi_c3);
					return;
				}
				return;
			} else if (common == C_NOTE_OFF) {
				inst->note_off(chan,
					       inst->midi_c2,
					       inst->midi_c3);
				return;
			} else if (common == C_PITCH_WHEEL) {
				inst->pitchwheel(chan,
						 inst->midi_c2,
						 inst->midi_c3);
				return;
			} else if (common == C_PROGRAM_CHANGE) {
				/* TODO: Test ! */
				inst->program_change(chan,
						     inst->midi_c2);
				return;
			} else if (common == C_POLYPHONIC_AFTERTOUCH) {
				/* TODO:  Test ! */
				inst->poly_aftertouch(chan,
						      inst->midi_c2,
						      inst->midi_c3);
				return;
			} else if (common == C_CHANNEL_AFTERTOUCH) {
				/* TODO:  Test ! */
				inst->channel_aftertouch(chan,
							 inst->midi_c2);
				return;
			} else if (common == C_CONTROL_CHANGE) {
				inst->control_change(chan,
						     inst->midi_c2,
						     inst->midi_c3);
				return;
			} else {
				/* Ignore unknown */
				return;
			}
		} else {
			if (inst->running_status_rx == 0) {
				/* Ignore data Byte if running status is  0 */
				return;
			} else {
				/* All 2 byte commands */
				if (inst->running_status_rx < 0xC0) {
					inst->third_byte_flag = 1;
					inst->midi_c2 = c;
					/* At this stage we have only 1 byte out of 2. */
					return;
				} else if (inst->running_status_rx < 0xE0) {
					/* All 1 byte commands */
					inst->midi_c2 = c;
					/*
					 * TODO: !! Process callback/delegate
					 * for two bytes command.
					 */
					return;
				} else if (inst->running_status_rx < 0xF0) {
					inst->third_byte_flag = 1;
					inst->midi_c2 = c;
				}
				/* !! */
				else if (inst->running_status_rx >= 0xF0) {
					if (inst->running_status_rx == 0xF2) {
						inst->running_status_rx = 0;
						inst->third_byte_flag = 1;
						inst->midi_c2 = c;
						return;
					} else if (inst->running_status_rx >=
						   0xF0) {
						if (inst->running_status_rx ==
						    0xF3
						    || inst->running_status_rx
						    == 0xF3) {
							inst->running_status_rx
							= 0;
							inst->midi_c2 = c;
							/*  TODO: !! Process callback for two bytes command. */
							return;
						} else {
							/* Ignore status */
							inst->running_status_rx
							= 0;
							return;
						}
					}
				}
			}
		} /*  global_3rd_byte_flag */
	} /* end of data bit 7 == 0 */
} /* End of SerialMidiReceiveParser */

/* EOF */

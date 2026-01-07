/**
 * @file midi1_serial.c
 * @brief MIDI USART implementation on Zephyr RTOS  
 *
 * @note 
 * MIDI USART implementation that also implements
 * MIDI "running status".  Not many example implementations 
 * do this but it's a very useful method to limit messages on 
 * serial UART MIDI channels that are only 31250 Baud and reduce
 * playing latency. It's essential when working with real gear. 
 * 
 *  The MIDI USART implementation for the Zephyr RTOS
 *  and uses the ring buffer and UART driver.
 * 
 *  TODO:  - Change the parser to accept a MIDI channel number or OMNI mode.
 *  TODO:  - Use the return values from uart_send( and return them.
 *  TODO:  - Extend callback functions to be channel aware (right now they are
 *  TODO:  - OMNI ALL --> i.e. all channels).
 * - Error handling for the parser right now it's ignored.
 *
 * Created in 2014 ported to Zephyr RTOS in 2024. 
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @updated 20260103
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include "midi1.h"
#include "midi1_serial.h"

/* Filled by the ISR routine */
K_MSGQ_DEFINE(midi_msgq, MSG_SIZE, MSGQ_SIZE, 4);

/*-----------------------------------------------------------------------*/
/* Global variables */
static uint8_t global_running_status_tx;
static uint8_t global_running_status_tx_count;
static uint8_t global_running_status_rx;
static uint8_t global_3rd_byte_flag;
static uint8_t global_midi_c2;
static uint8_t global_midi_c3;

/* 
 * Make sure there is a  "midi" in the device tree overlay. 
 * We'll cover the multi port MIDI stuff later. for now just one MIDI port is 
 * supported per board.   Also this is a UART device so make sure the UART
 * is enabled in the device prj.conf
 * TODO: _OLD_ version in the new version the caller needs to do this and
 * TODO: pass it as an argument
 */
#define UART_DEVICE_NODE DT_ALIAS(midi)
static const struct device *const midi = DEVICE_DT_GET(UART_DEVICE_NODE);

/*-----------------------------------------------------------------------*/
/* TODO: _OLD_ Global Function pointers for the delegate/callbacks */
void (*midi_note_on_delegate)(uint8_t note, uint8_t velocity);
void (*midi_note_off_delegate)(uint8_t note, uint8_t velocity);
void (*midi_control_change_delegate)(uint8_t controller, uint8_t value);
void (*realtime_handler_delegate)(uint8_t msg);
void (*midi_pitchwheel_delegate)(uint8_t lsb, uint8_t msb);

/* Private/hidden Prototype's for the ISR callback */
static void midi1_serial_isr_callback(const struct device *dev, void *user_data);
static void serial_isr_callback(const struct device *dev, void *user_data);


/**
 * Inits the serial USART with MIDI clock speed and 
 * registers delegates for the callbacks.
 * TODO: _NEW_ work in progress
 */
int midi1_serial_init(struct midi1_serial_inst *inst,
		     const struct device *uart_dev,
		     void(*note_on)(uint8_t, uint8_t),
		     void(*note_off)(uint8_t, uint8_t),
		     void(*control_change)(uint8_t, uint8_t),
		     void(*realtime)(uint8_t),
		     void(*pitchwheel)(uint8_t, uint8_t))
{
	inst->uart = uart_dev;

	inst->running_status_rx = 0;
	inst->third_byte_flag = 0;
	inst->midi_c2 = 0;
	inst->midi_c3 = 0;

	inst->running_status_tx = 0;
	inst->running_status_tx_count = 0;

	/* Assign delegate's */
	inst->note_on = note_on;
	inst->note_off = note_off;
	inst->control_change = control_change;
	inst->realtime = realtime;
	inst->pitchwheel = pitchwheel;

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
	return 0;
}

void SerialMidiInit(void (*note_on_handler_ptr)(uint8_t note, uint8_t velocity),
		    void (*note_off_handler_ptr)(uint8_t note,
						 uint8_t velocity),
		    void (*control_change_handler_ptr)(uint8_t controller,
						       uint8_t value),
		    void (*realtime_handler_delegate_ptr)(uint8_t msg),
		    void (*midi_pitchwheel_delegate_ptr)(uint8_t lsb,
							 uint8_t msb))
{
	/* Assign delegate's */
	midi_note_on_delegate = (void *)note_on_handler_ptr;
	midi_note_off_delegate = (void *)note_off_handler_ptr;
	midi_control_change_delegate = (void *)control_change_handler_ptr;
	realtime_handler_delegate = (void *)realtime_handler_delegate_ptr;
	midi_pitchwheel_delegate = (void *)midi_pitchwheel_delegate_ptr;

	/* Init the receive state machine */
	global_running_status_tx = 0;
	global_running_status_tx_count = 0;
	global_running_status_rx = 0;
	global_3rd_byte_flag = 0;
	global_midi_c2 = 0;
	global_midi_c3 = 0;

	/* TODO add Zephyr specific init stuff */
	if (!device_is_ready(midi)) {
		printk("UART device not found!");
		return;
	}
	int ret =
	    uart_irq_callback_user_data_set(midi, serial_isr_callback, NULL);
	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk
			    ("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk
			    ("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return;
	}
	uart_irq_rx_enable(midi);
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

void SerialMidiNoteON(uint8_t channel, uint8_t key, uint8_t velocity)
{
	if ((C_NOTE_ON | channel) != global_running_status_tx) {
		uart_poll_out(midi, C_NOTE_ON | channel);
		global_running_status_tx = C_NOTE_ON | channel;
	}
	uart_poll_out(midi, key);
	uart_poll_out(midi, velocity);
}

void midi1_serial_note_off(struct midi1_serial_inst *inst, uint8_t channel, uint8_t key, uint8_t velocity)
{
	if ((C_NOTE_ON | channel) != inst->running_status_tx) {
		uart_poll_out(inst->uart, C_NOTE_OFF | channel);
		inst->running_status_tx = C_NOTE_OFF | channel;
	}
	uart_poll_out(inst->uart, key);
	uart_poll_out(inst->uart, velocity);

}

void SerialMidiNoteOFF(uint8_t channel, uint8_t key, uint8_t velocity)
{
	if ((C_NOTE_ON | channel) != global_running_status_tx) {
		uart_poll_out(midi, C_NOTE_OFF | channel);
		global_running_status_tx = C_NOTE_OFF | channel;
	}
	uart_poll_out(midi, key);
	uart_poll_out(midi, velocity);
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
		uart_poll_out(midi, C_CONTROL_CHANGE | channel);
		inst->running_status_tx = C_CONTROL_CHANGE | channel;
	}
	/* If we don't have running status send out the status byte. */
	else if ((C_CONTROL_CHANGE | channel) != inst->running_status_tx) {
		uart_poll_out(midi, C_CONTROL_CHANGE | channel);
		inst->running_status_tx = C_CONTROL_CHANGE | channel;
		inst->running_status_tx_count = 0;
	}
	/* We always send out controller and value */
	uart_poll_out(inst->uart, controller);
	uart_poll_out(inst->uart, val);
	inst->running_status_tx_count++;
}

void SerialMidiControlChange(uint8_t channel, uint8_t controller, uint8_t val)
{
	if (global_running_status_tx_count >= 16) {
		global_running_status_tx_count = 0;
		uart_poll_out(midi, C_CONTROL_CHANGE | channel);
		global_running_status_tx = C_CONTROL_CHANGE | channel;
	}
	/* If we don't have running status send out the status byte. */
	else if ((C_CONTROL_CHANGE | channel) != global_running_status_tx) {
		uart_poll_out(midi, C_CONTROL_CHANGE | channel);
		global_running_status_tx = C_CONTROL_CHANGE | channel;
		global_running_status_tx_count = 0;
	}
	/* We always send out controller and value */
	uart_poll_out(midi, controller);
	uart_poll_out(midi, val);
	global_running_status_tx_count++;
}

void midi1_serial_channelaftertouch(struct midi1_serial_inst *inst,
				 uint8_t channel,
				 uint8_t val)
{
	if ((C_CHANNEL_AFTERTOUCH | channel) != inst->running_status_tx) {
		uart_poll_out(midi, C_CHANNEL_AFTERTOUCH | channel);
		inst->running_status_tx = C_CHANNEL_AFTERTOUCH | channel;
	}
	uart_poll_out(inst->uart, val);
}

void SerialMidiChannelAfterTouch(uint8_t channel, uint8_t val)
{
	if ((C_CHANNEL_AFTERTOUCH | channel) != global_running_status_tx) {
		uart_poll_out(midi, C_CHANNEL_AFTERTOUCH | channel);
		global_running_status_tx = C_CHANNEL_AFTERTOUCH | channel;
	}
	uart_poll_out(midi, val);
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

void SerialMidiModWheel(uint8_t channel, uint16_t val)
{
	SerialMidiControlChange(channel, CTL_MSB_MODWHEEL,
				~(CHANNEL_VOICE_MASK) & (val >> 7));
	SerialMidiControlChange(channel, CTL_LSB_MODWHEEL,
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

void SerialMidiPitchWheel(uint8_t channel, uint16_t val)
{
	if (global_running_status_tx != (C_PITCH_WHEEL | channel)) {
		uart_poll_out(midi, C_PITCH_WHEEL | channel);
		global_running_status_tx = C_PITCH_WHEEL | channel;
	}
	// Value is 14 bits so need to shift 7
	uart_poll_out(midi, val & ~(CHANNEL_VOICE_MASK));	// LSB
	uart_poll_out(midi, (val >> 7) & ~(CHANNEL_VOICE_MASK));	// MSB
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

void SerialMidiTimingClock(void)
{
	uart_poll_out(midi, RT_TIMING_CLOCK);
}

void midi1_serial_start(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_START);
}

void SerialMidiStart(void)
{
	uart_poll_out(midi, RT_START);
}

void midi1_serial_continue(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_CONTINUE);
}

void SerialMidiContinue(void)
{
	uart_poll_out(midi, RT_CONTINUE);
}

void midi1_serial_stop(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_STOP);
}

void SerialMidiStop(void)
{
	uart_poll_out(midi, RT_STOP);
}

void midi1_serial_active_sensing(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_ACTIVE_SENSING);
}

void SerialMidiActive_Sensing(void)
{
	uart_poll_out(midi, RT_ACTIVE_SENSING);
}

void midi1_serial_reset(struct midi1_serial_inst *inst)
{
	uart_poll_out(inst->uart, RT_RESET);
}

void SerialMidiReset(void)
{
	uart_poll_out(midi, RT_RESET);
}


/**
 * @brief MIDI Receive parser implementation - Interrupt Service Routine
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

/*
 * MIDI Receive parser implementation - Interrupt Service Routine
 * we use a Message Queue FIFO buffer to store the incoming MIDI messages
 * TODO: _OLD_ implementation did not use user_data
 */
static void serial_isr_callback(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(midi)) {
		return;
	}

	if (!uart_irq_rx_ready(midi)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(midi, &c, 1) == 1) {
		if (k_msgq_put(&midi_msgq, &c, K_NO_WAIT) != 0) {
			/* Message queue is full, handle overflow if necessary */
		}
	}
}

#if 0
/* TODO: DEAD CODE there was no need to hide access to the msgq */
/**
 * @brief get a message from the msgq filled by the ISR
 * @param *inst instance struct
 * @param *data pointer to buffer
 * @note TODO: _NEW_ version needs testing
 */
static int midi1_serial_msgq_get(struct midi1_serial_inst *inst, uint8_t *data)
{
	return k_msgq_get(&inst->msgq, data, K_FOREVER);
}


/*
 * parser is using the message queue (not used)
 * TODO: _OLD_ need to be replaced this version is using the global msgq
 */
static int midi_msgq_get(uint8_t *data)
{
	return k_msgq_get(&midi_msgq, data, K_FOREVER);
}
#endif

/**
 * @brief Parse one byte at a time for the MIDI parsing.
 *
 * @note Then callback functions are called for each complete MIDI message.
 *
 * @note This function is really long as the MIDI parsing is done
 * @note byte per byte and it took several tests to get the parsing
 * @note done right.  I tried to create a statemachine earlier but
 * @note failed so I have sticked to this proven implementation.
 *
 * TODO: _NEW_ version.
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
		/* Valid message received */
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
		/* is it a real-time message?  0xF8 up to 0xFF */
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
			 * TODO: We don't care about the input channel (OMNI) for now.
			 * so what we are doing here is to set the lower 4 bits to 0.
			 */
			inst->running_status_rx &= 0xF0;
			if (inst->running_status_rx == C_NOTE_ON) {
				if (inst->midi_c3 == 0) {
					/*
					 * A lot of MIDI implementation use velocity zero "note on"
					 * as a "note-off".  Other do use a note off and the note off velocity
					 * actually can be used to alter the sound of the note off.
					 */
					inst->note_off(inst->midi_c2,
						       inst->midi_c3);
					return;
				} else {
					inst->note_on(inst->midi_c2,
						      inst->midi_c3);
					return;
				}
				return;
			} else if (inst->running_status_rx == C_NOTE_OFF) {
				inst->note_off(inst->midi_c2,
					       inst->midi_c3);
				return;
			} else if (inst->running_status_rx == C_PITCH_WHEEL) {
				inst->pitchwheel(inst->midi_c2,
						 inst->midi_c3);
				return;
			} else if (inst->running_status_rx == C_PROGRAM_CHANGE) {
				/* TODO:  implement call callback! */
				return;
			} else if (inst->running_status_rx ==
				   C_POLYPHONIC_AFTERTOUCH) {
				/* TODO:  implement call callback! */
				return;
			} else if (inst->running_status_rx ==
				   C_CHANNEL_AFTERTOUCH) {
				/* TODO:  implement call callback! */
				return;
			} else if (inst->running_status_rx == C_CONTROL_CHANGE) {
				inst->control_change(inst->midi_c2,
						     inst->midi_c3);
				/* TODO:  implement call callback! */
				return;
			} else {
				/* Ignore */
				return;
			}
		} else {
			if (inst->running_status_rx == 0) {
				/* Ignore data Byte if running status is  0 */
				return;
			} else {
				if (inst->running_status_rx < 0xC0) {	/* All 2 byte commands */
					inst->third_byte_flag = 1;
					inst->midi_c2 = c;
					/* At this stage we have only 1 byte out of 2. */
					return;
				} else if (inst->running_status_rx < 0xE0) {	/* All 1 byte commands */
					inst->midi_c2 = c;
					/* TODO: !! Process callback/delegate for two bytes command. */
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
		}		/*  global_3rd_byte_flag */
	}			/* end of data bit 7 == 0 */
	
}				/* End of SerialMidiReceiveParser */




/*
 * We parse one byte at a time for the MIDI parsing. Then callback functions
 * are called for each complete MIDI message
 * TODO: _OLD_ version not instance aware.
 */
void SerialMidiReceiveParser(void)
{
	uint8_t c;

	/*  
	 * Read only _one_ byte from the circular FIFO input buffer
	 * This buffer is filled by the ISR routine on receipt of
	 * data on the port.
	 */
	if (k_msgq_get(&midi_msgq, &c, K_FOREVER) != 0) {
		return;
	} else {
		/* Valid message received */
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
		/* is it a real-time message?  0xF8 up to 0xFF */
		if (c >= 0xF8) {
			realtime_handler_delegate(c);
			return;
		} else {
			global_running_status_rx = c;
			global_3rd_byte_flag = 0;
			/* Is this a tune request */
			if (c == SYSTEM_TUNE_REQUEST) {
				global_midi_c2 = c;	/*  Store in FIFO. */
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
		if (global_3rd_byte_flag == 1) {
			global_3rd_byte_flag = 0;
			global_midi_c3 = c;

			/* 
			 * TODO: We don't care about the input channel (OMNI) for now. 
			 * so what we are doing here is to set the lower 4 bits to 0.
			 */
			global_running_status_rx &= 0xF0;
			if (global_running_status_rx == C_NOTE_ON) {
				if (global_midi_c3 == 0) {
					/* 
					 * A lot of MIDI implementation use velocity zero "note on"
					 * as a "note-off".  Other do use a note off and the note off velocity
					 * actually can be used to alter the sound of the note off.
					 */
					midi_note_off_delegate(global_midi_c2,
							       global_midi_c3);
					return;
				} else {
					midi_note_on_delegate(global_midi_c2,
							      global_midi_c3);
					return;
				}
				return;
			} else if (global_running_status_rx == C_NOTE_OFF) {
				midi_note_off_delegate(global_midi_c2,
						       global_midi_c3);
				return;
			} else if (global_running_status_rx == C_PITCH_WHEEL) {
				midi_pitchwheel_delegate(global_midi_c2,
							 global_midi_c3);
				return;
			} else if (global_running_status_rx == C_PROGRAM_CHANGE) {
				return;
			} else if (global_running_status_rx ==
				   C_POLYPHONIC_AFTERTOUCH) {
				return;
			} else if (global_running_status_rx ==
				   C_CHANNEL_AFTERTOUCH) {
				return;
			} else if (global_running_status_rx == C_CONTROL_CHANGE) {
				midi_control_change_delegate(global_midi_c2,
							     global_midi_c3);
				return;
			} else {
				/* Ignore */
				return;
			}
		} else {
			if (global_running_status_rx == 0) {
				/* Ignore data Byte if running status is  0 */
				return;
			} else {
				if (global_running_status_rx < 0xC0) {	/* All 2 byte commands */
					global_3rd_byte_flag = 1;
					global_midi_c2 = c;
					// At this stage we have only 1 byte out of 2.
					return;
				} else if (global_running_status_rx < 0xE0) {	/* All 1 byte commands */
					global_midi_c2 = c;
					/* TODO: !! Process callback/delegate for two bytes command. */
					return;
				} else if (global_running_status_rx < 0xF0) {
					global_3rd_byte_flag = 1;
					global_midi_c2 = c;
				}
				/* !! */
				else if (global_running_status_rx >= 0xF0) {
					if (global_running_status_rx == 0xF2) {
						global_running_status_rx = 0;
						global_3rd_byte_flag = 1;
						global_midi_c2 = c;
						return;
					} else if (global_running_status_rx >=
						   0xF0) {
						if (global_running_status_rx ==
						    0xF3
						    || global_running_status_rx
						    == 0xF3) {
							global_running_status_rx
							    = 0;
							global_midi_c2 = c;
							/*  TODO: !! Process callback for two bytes command. */
							return;
						} else {
							/* Ignore status */
							global_running_status_rx
							    = 0;
							return;
						}
					}
				}
			}
		}		/*  global_3rd_byte_flag */
	}			/* end of data bit 7 == 0 */

}				/* End of SerialMidiReceiveParser */

/* EOF */

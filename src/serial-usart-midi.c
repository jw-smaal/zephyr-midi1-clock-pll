/**
 * serial-usart-midi.c
 * 
 * MIDI USART implementation that also implements
 * MIDI "running status".  Not many implementation do this but 
 * it's a very useful method to limit messages on serial
 * UART MIDI channels that are only 31250 Baud and reduce
 * playing latency. 
 * 
 *  The MIDI USART implementation for the Zephyr RTOS
 *  and uses the ring buffer and UART driver.
 * 
 *  TODO:  - Change the parser to accept a MIDI channel number or OMNI mode. 
 * - Error handling for the parser right now it's ignored. 
 *
 * Created in 2014 ported to Zephyr RTOS in 2024. 
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include "serial-usart-midi.h"

#define MSGQ_SIZE 128
#define MSG_SIZE sizeof(uint8_t)

K_MSGQ_DEFINE(midi_msgq, MSG_SIZE, MSGQ_SIZE, 4);

/* 
 * Make sure there is a  "midi" in the device tree overlay. 
 * We'll cover the multi port MIDI stuff later. for now just one MIDI port is 
 * supported per board.   
 * Also this is a UART device so make sure the UART is enabled in the device tree.
 * We have not tested any USB MIDI HID devices yet with this implementaiton. 
 */ 
#define UART_DEVICE_NODE DT_ALIAS(midi)
static const struct device *const midi = DEVICE_DT_GET(UART_DEVICE_NODE);

/*-----------------------------------------------------------------------*/
/* Global Function pointers for the delegate/callbacks */
void (*midi_note_on_delegate)(uint8_t note, uint8_t velocity);
void (*midi_note_off_delegate)(uint8_t note, uint8_t velocity);
void (*midi_control_change_delegate)(uint8_t controller, uint8_t value);
void (*realtime_handler_delegate)(uint8_t msg);
void (*midi_pitchwheel_delegate)(uint8_t lsb, uint8_t msb);


/**
 * Inits the serial USART with MIDI clock speed and 
 * registers delegates for the callbacks.
 */ 
void SerialMidiInit(void (*note_on_handler_ptr)(uint8_t note, uint8_t velocity),
                    void (*note_off_handler_ptr)(uint8_t note, uint8_t velocity),
                    void (*control_change_handler_ptr)(uint8_t controller, uint8_t value),
                    void (*realtime_handler_delegate_ptr)(uint8_t msg),
                    void (*midi_pitchwheel_delegate_ptr)(uint8_t lsb, uint8_t msb))
{
    /* Assign delegate's */
    midi_note_on_delegate = (void *)note_on_handler_ptr;
    midi_note_off_delegate = (void *)note_off_handler_ptr;
    midi_control_change_delegate = (void *)control_change_handler_ptr;    
	realtime_handler_delegate =  (void *)realtime_handler_delegate_ptr;
    midi_pitchwheel_delegate = (void *)midi_pitchwheel_delegate_ptr;

    /* Init the receive state machine */
    global_running_status_tx = 0;
    global_running_status_rx = 0;
    global_3rd_byte_flag = 0;
    global_midi_c2 = 0;
    global_midi_c3 = 0;

    /* TODO add Zephyr specific init stuff */
	if (!device_is_ready(midi)) {
		printk("UART device not found!");
		return;
	}
    int ret = uart_irq_callback_user_data_set(midi, serial_isr_callback, NULL);
    	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
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
void SerialMidiNoteON(uint8_t channel, uint8_t key, uint8_t velocity)
{
    if((C_NOTE_ON | channel) !=  global_running_status_tx) {    
        uart_poll_out(midi, C_NOTE_ON | channel);
        global_running_status_tx = C_NOTE_ON | channel;
    }
    uart_poll_out(midi, key);
    uart_poll_out(midi, velocity);
}


void SerialMidiNoteOFF(uint8_t channel, uint8_t key, uint8_t velocity)
{
   if((C_NOTE_ON | channel) !=  global_running_status_tx) {    
        uart_poll_out(midi, C_NOTE_OFF | channel);
        global_running_status_tx = C_NOTE_OFF | channel;
    }
    uart_poll_out(midi, key);
    uart_poll_out(midi, velocity);
}


void SerialMidiControlChange(uint8_t channel, uint8_t controller, uint8_t val)
{
    if((C_CONTROL_CHANGE | channel) !=  global_running_status_tx) {
        uart_poll_out(midi, C_CONTROL_CHANGE | channel);
        global_running_status_tx = C_CONTROL_CHANGE | channel;
    }
    uart_poll_out(midi, controller);
    uart_poll_out(midi, val);
}


void SerialMidiChannelAfterTouch(uint8_t channel, uint8_t val)
{
    if((C_CHANNEL_AFTERTOUCH | channel) !=  global_running_status_tx) {
        uart_poll_out(midi, C_CHANNEL_AFTERTOUCH | channel);
        global_running_status_tx = C_CHANNEL_AFTERTOUCH | channel;
    }
    uart_poll_out(midi, val);
}


/**
 * Modulation Wheel both LSB and MSB
 * range: 0 --> 16383
 */
void SerialMidiModWheel(uint8_t channel, uint16_t val)
{
    SerialMidiControlChange(channel, CTL_MSB_MODWHEEL,  ~(CHANNEL_VOICE_MASK) & (val>>7));
    SerialMidiControlChange(channel, CTL_LSB_MODWHEEL,  ~(CHANNEL_VOICE_MASK) & val);
}


/**
 * PitchWheel is always with 14 bit value.
 *       LOW   MIDDLE   HIGH
 * range: 0 --> 8192  --> 16383
 */
void SerialMidiPitchWheel(uint8_t channel, uint16_t val)
{
    if (global_running_status_tx!= (C_PITCH_WHEEL | channel)) {
        uart_poll_out(midi, C_PITCH_WHEEL | channel);
        global_running_status_tx = C_PITCH_WHEEL | channel;
    }
    // Value is 14 bits so need to shift 7
    uart_poll_out(midi, val & ~(CHANNEL_VOICE_MASK));        // LSB
    uart_poll_out(midi, (val>>7) & ~(CHANNEL_VOICE_MASK));   // MSB
}

void SerialMidiTimingClock(void)
{
    uart_poll_out(midi, RT_TIMING_CLOCK);
}


void SerialMidiStart(void)
{
    uart_poll_out(midi, RT_START);
}


void SerialMidiContinue(void)
{
    uart_poll_out(midi, RT_CONTINUE);
}


void SerialMidiStop(void)
{
    uart_poll_out(midi, RT_STOP);
}


void SerialMidiActive_Sensing(void)
{
    uart_poll_out(midi, RT_ACTIVE_SENSING);
}


void SerialMidiReset(void)
{
    uart_poll_out(midi, RT_RESET);
}


/* 
 * MIDI Receive parser implementation - Interrupt Service Routine
 * we use a Message Queue FIFO buffer to store the incoming MIDI messages 
 */
void serial_isr_callback(const struct device *dev, void *user_data)
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


/* 
 * Improved version using the message queue 
 */
int midi_msgq_get(uint8_t *data)
{
    return k_msgq_get(&midi_msgq, data, K_FOREVER);
}


/* 
 * We parse one byte at a time for the MIDI parsing. Then callback functions are called
 * for each complete message 
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
	    printk("%2X ", c);
    }


	/* 
     * Future implementation option
     * To allow software MIDI THRU (kind of with some processing delay)
     * simply write what is received to the output. 
     * uart_poll_out(midi, c);
     */

    /* Check if bit7 = 1 */
    if ( c & CHANNEL_VOICE_MASK ) {
	    /* if (! (c & SYSTEM_REALTIME_MASK)) */
		/* is it a real-time message?  0xF8 up to 0xFF */ 
        if (c >= 0xF8 ) {
			realtime_handler_delegate(c);
            return;
        }
        else {
            global_running_status_rx = c;
            global_3rd_byte_flag = 0;
            /* Is this a tune request */
            if(c == SYSTEM_TUNE_REQUEST) {
                global_midi_c2 = c; /*  Store in FIFO. */
                /* TODO: Process something. */
                return;
            }
            /* 
             * Do nothing
             * Ignore for now 
             */
            return;
        }
    }
    else {  /* Bit 7 == 0   (data) */
	    if(global_3rd_byte_flag == 1) {
            global_3rd_byte_flag = 0;
            global_midi_c3 = c;

			/* 
             * TODO: We don't care about the input channel (OMNI) for now. 
             * so what we are doing here is to set the lower 4 bits to 0.
             */
            global_running_status_rx &= 0xF0;
            if(global_running_status_rx == C_NOTE_ON){
				if(global_midi_c3 == 0 ) {
					/* 
                     * A lot of MIDI implementation use velocity zero "note on"
					 * as a "note-off".  Other do use a note off and the note off velocity
                     * actually can be used to alter the sound of the note off.
                     */ 
					midi_note_off_delegate(global_midi_c2, global_midi_c3);
					return;
				}  
				else {
	            	midi_note_on_delegate(global_midi_c2, global_midi_c3);
					return;
				}
				return;
            }
            else if(global_running_status_rx == C_NOTE_OFF) {
                midi_note_off_delegate(global_midi_c2, global_midi_c3);
                return;
            }
			else if(global_running_status_rx == C_PITCH_WHEEL) {
				midi_pitchwheel_delegate(global_midi_c2, global_midi_c3);
				return; 
			}
			else if(global_running_status_rx == C_PROGRAM_CHANGE) {
				return; 
			}
			else if(global_running_status_rx ==  C_POLYPHONIC_AFTERTOUCH) {
				return; 
			}
			else if(global_running_status_rx ==  C_CHANNEL_AFTERTOUCH) {
				return; 
			}
            else if(global_running_status_rx == C_CONTROL_CHANGE) {
                midi_control_change_delegate(global_midi_c2, global_midi_c3);
                return;
            }
            else {
                /* Ignore */
                return;
            }   
        }
        else {
            if(global_running_status_rx == 0) {
                /* Ignore data Byte if running status is  0 */
                return;
            }
            else {
                if (global_running_status_rx < 0xC0) { /* All 2 byte commands */
                    global_3rd_byte_flag = 1;
                    global_midi_c2 = c;
                    // At this stage we have only 1 byte out of 2.
                    return;
                }
                else if (global_running_status_rx < 0xE0) {    /* All 1 byte commands */
                    global_midi_c2 = c;
                    /* TODO: !! Process callback/delegate for two bytes command. */
                    return;
                }
                else if ( global_running_status_rx < 0xF0){
                    global_3rd_byte_flag = 1;
                    global_midi_c2 = c;
                }
				/* !! */ 
                else if ( global_running_status_rx >= 0xF0) {
                    if (global_running_status_rx == 0xF2) {
                        global_running_status_rx = 0;
                        global_3rd_byte_flag = 1;
                        global_midi_c2 = c;
                        return;
                    }
                    else if (global_running_status_rx >= 0xF0 ){
                        if(global_running_status_rx == 0xF3 ||
                           global_running_status_rx == 0xF3 ) {
                            global_running_status_rx = 0;
                            global_midi_c2 = c;
                            /*  TODO: !! Process callback for two bytes command. */
                            return;
                        }
                        else {
                            /* Ignore status */ 
                            global_running_status_rx = 0;
                            return;
                        }
                    }
                }
            }  
        }  /*  global_3rd_byte_flag*/
    } /* end of data bit 7 == 0 */

} /* End of SerialMidiReceiveParser */


/* EOF */

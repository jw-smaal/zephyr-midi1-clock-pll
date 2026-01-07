/**
 * @file midi1_serial.h
 * @brief Serial USART implementation of MIDI1.0 for Zephyr 
 *
 * @note 
 * Created in 2014 initially for ATMEL MCU's then ported to 
 * ARM MBED targets, ported to Zephyr RTOS in 2024.
 * Adjusted in 2025 to work with UMP.  Changed to to support
 * multiple instances in 2026.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @updated 20250103
 * @updated 20260107
 *
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_SERIAL_H
#define MIDI1_SERIAL_H
/*-----------------------------------------------------------------------*/
#include <string.h>		/* Check if really needed ? */
#include <stdint.h>

/* MIDI1.0 definitions by Jan-Willem Smaal */
#include "midi1.h"

#define MIDI1_SERIAL_DEBUG 1
#define MSGQ_SIZE 128
#define MSG_SIZE sizeof(uint8_t)

/* TODO: Rebuilding this code to a instance based version */
struct midi1_serial_inst {
	const struct device *uart;
	
	/* RX parser state */
	uint8_t running_status_rx;
	uint8_t third_byte_flag;
	uint8_t midi_c2;
	uint8_t midi_c3;
	
	/* TX running status */
	uint8_t running_status_tx;
	uint8_t running_status_tx_count;
	
	/* Message queue filled by the ISR routine */
	struct k_msgq msgq;
	uint8_t msgq_buffer[MSGQ_SIZE];
	
	/* Callback delegates */
	void (*note_on)(uint8_t note, uint8_t velocity);
	void (*note_off)(uint8_t note, uint8_t velocity);
	void (*control_change)(uint8_t controller, uint8_t value);
	void (*realtime)(uint8_t msg);
	void (*pitchwheel)(uint8_t lsb, uint8_t msb);
};


/*-----------------------------------------------------------------------*/
/*  Function prototypes */


/**
 * @brief inits the MIDI serial subsystem for the inst instance
 *
 * @note also assigns delegates for callbacks.
 *
 * @param *inst pointer to a midi1_serial_inst struct
 * @param *note_on pointer to the callback function for a NOTE ON rx event
 * @param *note_off callback delegate function for a NOTE OFF rx event
 * @param *note_on  callback delegate function for a NOTE ON rx event
 * @param *control_change  callback function for a Control Change rx event
 * @param *realtime  callback delegate function for a system realtime rx event
 * @param *pitchwheel callback delegate function for a pitchwheel rx event
 * @return err success 0 failure -1
 */
int midi1_serial_init(struct midi1_serial_inst *inst,
		     const struct device *uart_dev,
		     void(*note_on)(uint8_t, uint8_t),
		     void(*note_off)(uint8_t, uint8_t),
		     void(*control_change)(uint8_t, uint8_t),
		     void(*realtime)(uint8_t),
		     void(*pitchwheel)(uint8_t, uint8_t));

void SerialMidiInit(void (*note_on_handler_ptr)(uint8_t note, uint8_t velocity),
		    void(*note_off_handler_ptr)(uint8_t note, uint8_t velocity),
		    void(*control_change_handler_ptr)(uint8_t controller,
						      uint8_t value),
		    void(*realtime_handler_ptr)(uint8_t msg),
		    void(*midi_pitchwheel_ptr)(uint8_t lsb, uint8_t msb));


/**
 * @brief this needs to be called in a loop to process the received MIDI1
 *
 * @note advice to run this in a seperate thread.   It's a blocking function so
 * @note no delay is required between calls.
 * @note This process then calls the delegate functions / callbacks like
 * void (*note_on)(uint8_t, unit8_t)
 *
 */
void midi1_serial_receiveparser(struct midi1_serial_inst *inst);
void SerialMidiReceiveParser(void);


/* Channel mode messages */
/* ___ _                       _   __  __         _
  / __| |_  __ _ _ _  _ _  ___| | |  \/  |___  __| |___
 | (__| ' \/ _` | ' \| ' \/ -_) | | |\/| / _ \/ _` / -_)
  \___|_||_\__,_|_||_|_||_\___|_| |_|  |_\___/\__,_\___|
 */

/**
 * @brief send a NOTE ON tx event via the instance inst
 * @param *inst pointer to a midi1_serial_inst struct
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param key MIDI key number
 * @param velocity NOTE on velocity
 */
void midi1_serial_note_on(struct midi1_serial_inst *inst,
			  uint8_t channel,
			  uint8_t key,
			  uint8_t velocity);
void SerialMidiNoteON(uint8_t channel, uint8_t key, uint8_t velocity);

/**
 * @brief send a NOTE OFF tx event via the instance inst
 * @param *inst pointer to a midi1_serial_inst struct
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param key MIDI key number
 * @param velocity NOTE OFF velocity (not used by many...)
 */
void midi1_serial_note_off(struct midi1_serial_inst *inst,
			   uint8_t channel,
			   uint8_t key,
			   uint8_t velocity);
void SerialMidiNoteOFF(uint8_t channel, uint8_t key, uint8_t velocity);

/**
 * @brief send a Control Change tx event via the instance inst
 * @param *inst pointer to a midi1_serial_inst struct
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param controller MIDI controller value
 * @param value MIDI value
 */
void midi1_serial_control_change(struct midi1_serial_inst *inst,
				 uint8_t channel,
				 uint8_t controller,
				 uint8_t val);
void SerialMidiControlChange(uint8_t channel, uint8_t controller, uint8_t val);

/**
 * @brief send a Channel aftertouch tx event via the instance inst
 * @param *inst pointer to a midi1_serial_inst struct
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param value MIDI value
 */
void midi1_serial_channelaftertouch(struct midi1_serial_inst *inst,
				    uint8_t channel,
				    uint8_t val);
void SerialMidiChannelAfterTouch(uint8_t channel, uint8_t val);

/**
 * @brief send a ModWheel (MSB and LSB) via the instance inst
 *
 * @note Modulation Wheel both LSB and MSB
 * @note range: 0 --> 16383
 * @param *inst pointer to a midi1_serial_inst struct
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param value MIDI value
 */
void midi1_serial_modwheel(struct midi1_serial_inst *inst,
			   uint8_t channel,
			   uint16_t val);
void SerialMidiModWheel(uint8_t channel, uint16_t val);

/**
 * @brief send a Pitchwheel change (MSB and LSB) via the instance inst
 *
 * @note PitchWheel is always with 14 bit value.
 * @note       LOW   MIDDLE   HIGH
 * @note range: 0 --> 8192  --> 16383
 *
 * @param *inst pointer to a midi1_serial_inst struct
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param value MIDI value
 */
void SerialMidiPitchWheel(uint8_t channel, uint16_t val);

/* System Common messages */
/*___         _                ___
 / __|_  _ __| |_ ___ _ __    / __|___ _ __  _ __  ___ _ _
 \__ \ || (_-<  _/ -_) '  \  | (__/ _ \ '  \| '  \/ _ \ ' \
 |___/\_, /__/\__\___|_|_|_|  \___\___/_|_|_|_|_|_\___/_||_|
 */

/**
 * @brief send MIDI1 timing clock
 *
 * @param *inst pointer to a midi1_serial_inst struct
 */
void midi1_serial_timingclock(struct midi1_serial_inst *inst);
void SerialMidiTimingClock(void);

/**
 * @brief send MIDI1 start
 *
 * @param *inst pointer to a midi1_serial_inst struct
 */
void midi1_serial_start(struct midi1_serial_inst *inst);
void SerialMidiStart(void);

/**
 * @brief send MIDI1 continue
 *
 * @param *inst pointer to a midi1_serial_inst struct
 */
void midi1_serial_continue(struct midi1_serial_inst *inst);
void SerialMidiContinue(void);

/**
 * @brief send MIDI1 stop
 *
 * @param *inst pointer to a midi1_serial_inst struct
 */
void midi1_serial_stop(struct midi1_serial_inst *inst);
void SerialMidiStop(void);

/**
 * @brief send MIDI1 active sense
 *
 * @param *inst pointer to a midi1_serial_inst struct
 */
void midi1_serial_active_sensing(struct midi1_serial_inst *inst);
void SerialMidiActive_Sensing(void);

/**
 * @brief send MIDI1 RESET
 *
 * @param *inst pointer to a midi1_serial_inst struct
 */
void midi1_serial_reset(struct midi1_serial_inst *inst);
void SerialMidiReset(void);


#endif				/* MIDI1_SERIAL_H */
/* EOF */

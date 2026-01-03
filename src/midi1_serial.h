/**
 * @file midi1_serial.h
 * @brief Serial USART implementation of MIDI1.0 for Zephyr 
 *
 * @note 
 * Created in 2014 initially for ATMEL MCU's then ported to 
 * ARM MBED targets, finallyported to Zephyr RTOS in 2024. 
 * adjusted in 2025 to work with UMP
 *
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @updated 20250103
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

/*-----------------------------------------------------------------------*/
/*  Function prototypes */
void SerialMidiReceiveParser(void);

/* During the SerialMidiInit the delegate callback functions need to be assigned */
void SerialMidiInit(void (*note_on_handler_ptr)(uint8_t note, uint8_t velocity),
		    void(*note_off_handler_ptr)(uint8_t note, uint8_t velocity),
		    void(*control_change_handler_ptr)(uint8_t controller,
						      uint8_t value),
		    void(*realtime_handler_ptr)(uint8_t msg),
		    void(*midi_pitchwheel_ptr)(uint8_t lsb, uint8_t msb));

/* Channel mode messages */
void SerialMidiNoteON(uint8_t channel, uint8_t key, uint8_t velocity);
void SerialMidiNoteOFF(uint8_t channel, uint8_t key, uint8_t velocity);
void SerialMidiControlChange(uint8_t channel, uint8_t controller, uint8_t val);
void SerialMidiPitchWheel(uint8_t channel, uint16_t val);
void SerialMidiModWheel(uint8_t channel, uint16_t val);
void SerialMidiChannelAfterTouch(uint8_t channel, uint8_t val);

/* System Common messages */
void SerialMidiTimingClock(void);
void SerialMidiStart(void);
void SerialMidiContinue(void);
void SerialMidiStop(void);
void SerialMidiActive_Sensing(void);
void SerialMidiReset(void);

/* Prototype for the ISR callback */
void serial_isr_callback(const struct device *dev, void *user_data);

#endif				/* MIDI1_SERIAL_H */
/* EOF */

/**
 * midi1.c
 *
 * Created in 2014 ported to Zephyr RTOS in 2024. 
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include "midi1.h"


struct midi_ump Midi1NoteON(uint8_t channel, uint8_t key, uint8_t velocity){
	uint8_t status = C_NOTE_ON | (channel & 0x0F);
	
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP, status, note, velocity);
}


struct midi_ump Midi1NoteOFF(uint8_t channel, uint8_t key, uint8_t velocity){
	uint8_t status = C_NOTE_OFF | (channel & 0x0F);
	
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP, status, note, velocity);
}


struct midi_ump Midi1ControlChange(uint8_t channel,
								   uint8_t controller,
								   uint8_t val)
{
	uint8_t status = C_CONTROL_CHANGE | (channel & 0x0F);
	
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   status,
								   controller,
								   val);
}


#ifdef OLD_CODE
/*
 * All functions related to sending MIDI messages to the serial USART
 */
void Midi1NoteON(uint8_t channel, uint8_t key, uint8_t velocity)
{
    if((C_NOTE_ON | channel) !=  global_running_status_tx) {    
        //uart_poll_out(midi, C_NOTE_ON | channel);
        global_running_status_tx = C_NOTE_ON | channel;
    }
    //uart_poll_out(midi, key);
    //uart_poll_out(midi, velocity);
}


void Midi1NoteOFF(uint8_t channel, uint8_t key, uint8_t velocity)
{
   if((C_NOTE_ON | channel) !=  global_running_status_tx) {    
        //uart_poll_out(midi, C_NOTE_OFF | channel);
        global_running_status_tx = C_NOTE_OFF | channel;
    }
    //uart_poll_out(midi, key);
    //uart_poll_out(midi, velocity);
}


void Midi1ControlChange(uint8_t channel, uint8_t controller, uint8_t val)
{
    if((C_CONTROL_CHANGE | channel) !=  global_running_status_tx) {
        //uart_poll_out(midi, C_CONTROL_CHANGE | channel);
        global_running_status_tx = C_CONTROL_CHANGE | channel;
    }
    //uart_poll_out(midi, controller);
    //uart_poll_out(midi, val);
}


void Midi1ChannelAfterTouch(uint8_t channel, uint8_t val)
{
    if((C_CHANNEL_AFTERTOUCH | channel) !=  global_running_status_tx) {
        //uart_poll_out(midi, C_CHANNEL_AFTERTOUCH | channel);
        global_running_status_tx = C_CHANNEL_AFTERTOUCH | channel;
    }
    //uart_poll_out(midi, val);
}


/**
 * Modulation Wheel both LSB and MSB
 * range: 0 --> 16383
 */
void Midi1ModWheel(uint8_t channel, uint16_t val)
{
    SerialMidiControlChange(channel, CTL_MSB_MODWHEEL,  ~(CHANNEL_VOICE_MASK) & (val>>7));
    SerialMidiControlChange(channel, CTL_LSB_MODWHEEL,  ~(CHANNEL_VOICE_MASK) & val);
}


/**
 * PitchWheel is always with 14 bit value.
 *       LOW   MIDDLE   HIGH
 * range: 0 --> 8192  --> 16383
 */
void Midi1PitchWheel(uint8_t channel, uint16_t val)
{
    if (global_running_status_tx!= (C_PITCH_WHEEL | channel)) {
        //uart_poll_out(midi, C_PITCH_WHEEL | channel);
        global_running_status_tx = C_PITCH_WHEEL | channel;
    }
    //// Value is 14 bits so need to shift 7
    //uart_poll_out(midi, val & ~(CHANNEL_VOICE_MASK));        // LSB
    //uart_poll_out(midi, (val>>7) & ~(CHANNEL_VOICE_MASK));   // MSB
}

void Midi1TimingClock(void)
{
    //uart_poll_out(midi, RT_TIMING_CLOCK);
}


void Midi1Start(void)
{
    //uart_poll_out(midi, RT_START);
}


void Midi1Continue(void)
{
    //uart_poll_out(midi, RT_CONTINUE);
}


void Midi1Stop(void)
{
   //uart_poll_out(midi, RT_STOP);
}


void Midi1Active_Sensing(void)
{
    //uart_poll_out(midi, RT_ACTIVE_SENSING);
}


void Midi1Reset(void)
{
    //uart_poll_out(midi, RT_RESET);
}
#endif

/* EOF */

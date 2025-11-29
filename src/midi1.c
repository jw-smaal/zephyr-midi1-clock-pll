/**
 * midi1.c
 *
 * Created in 2014 ported to Zephyr RTOS in 2024. 
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/audio/midi.h>
#include "midi1.h"


struct midi_ump Midi1NoteON(uint8_t channel, uint8_t key, uint8_t velocity){
	uint8_t status = C_NOTE_ON | (channel & 0x0F);
	
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   /* command nibble: Note On */
								   (C_NOTE_ON >> 4),
								   channel,    /* channel number */
								   key,        /* note number */
								   velocity);  /* velocity */
}
	


struct midi_ump Midi1NoteOFF(uint8_t channel, uint8_t key, uint8_t velocity){
	uint8_t status = C_NOTE_OFF | (channel & 0x0F);

	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   /* command nibble: Note On */
								   (C_NOTE_OFF >> 4),
								   channel,    /* channel number */
								   key,        /* note number */
								   velocity);  /* velocity */
}


struct midi_ump Midi1ControlChange(uint8_t channel,
								   uint8_t controller,
								   uint8_t val)
{
	uint8_t status = C_CONTROL_CHANGE | (channel & 0x0F);
	
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   /* command nibble: Control change. */
								   (C_CONTROL_CHANGE >> 4) ,
								   channel,    	/* channel number */
								   controller, 	/* controller number */
								   val);  		/* velocity */
}


struct midi_ump Midi1ChannelAfterTouch(uint8_t channel, uint8_t val)
{
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   /* command
									nibble: Control change. */
								   (C_CHANNEL_AFTERTOUCH  >> 4) ,
								   channel,    	/* channel number */
								   (C_CHANNEL_AFTERTOUCH  >> 4) ,
								   val);  		/* aftertouch value */
}


struct midi_ump Midi1ModWheel(uint8_t channel, uint16_t val){
	//Midi1ControlChange(channel, CTL_MSB_MODWHEEL,  ~(CHANNEL_VOICE_MASK) & (val>>7));
	//Midi1ControlChange(channel, CTL_LSB_MODWHEEL,  ~(CHANNEL_VOICE_MASK) & val);
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   (C_PITCH_WHEEL >> 4),
								   channel & 0x0F,
								   ~(CHANNEL_VOICE_MASK) & val,
								   ~(CHANNEL_VOICE_MASK) & (val>>7));
}


struct midi_ump Midi1PitchWheel(uint8_t channel, uint16_t val){
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   (C_PITCH_WHEEL >> 4),
								   channel & 0x0F,
								   ~(CHANNEL_VOICE_MASK) & val,
								   ~(CHANNEL_VOICE_MASK) & (val>>7));
}


//void Midi1TimingClock(void)
//{
//    //uart_poll_out(midi, RT_TIMING_CLOCK);
//}
//
//
//void Midi1Start(void)
//{
//    //uart_poll_out(midi, RT_START);
//}
//
//
//void Midi1Continue(void)
//{
//    //uart_poll_out(midi, RT_CONTINUE);
//}
//
//
//void Midi1Stop(void)
//{
//   //uart_poll_out(midi, RT_STOP);
//}
//
//
//void Midi1Active_Sensing(void)
//{
//    //uart_poll_out(midi, RT_ACTIVE_SENSING);
//}
//
//
//void Midi1Reset(void)
//{
//    //uart_poll_out(midi, RT_RESET);
//}


/* EOF */

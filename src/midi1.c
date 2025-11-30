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


/**
 * -- == Channel messages == --
 */
struct midi_ump Midi1NoteON(uint8_t channel, uint8_t key, uint8_t velocity)
{
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   (C_NOTE_ON >> 4),
								   channel & 0x0F,
								   key & MIDI_DATA,
								   velocity & MIDI_DATA);
}


struct midi_ump Midi1NoteOFF(uint8_t channel, uint8_t key, uint8_t velocity)
{
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   (C_NOTE_OFF >> 4),
								   channel & 0x0F,
								   key & MIDI_DATA,
								   velocity & MIDI_DATA);
}


struct midi_ump Midi1ControlChange(uint8_t channel,
								   uint8_t controller,
								   uint8_t val)
{
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   (C_CONTROL_CHANGE >> 4) ,
								   channel & 0x0F,
								   controller & MIDI_DATA,
								   val & MIDI_DATA);
}


/*
 * Channel aftertouch is not a control change!!! 
 */
struct midi_ump Midi1ChannelAfterTouch(uint8_t channel, uint8_t val)
{
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   (C_CHANNEL_AFTERTOUCH >> 4) ,
								   channel & 0x0F,
								   val & MIDI_DATA,
								   0);
}


/*
 * Even though most keybeds don't send it; a lot of synths
 * can respond to polyphonic aftertouch.
 */
struct midi_ump Midi1PolyAfterTouch(uint8_t channel,
									uint8_t key,
									uint8_t val)
{
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   (C_POLYPHONIC_AFTERTOUCH >> 4) ,
								   channel & 0x0F,
								   key & MIDI_DATA,
								   val & MIDI_DATA);
}


/**
 * Mod wheel has both MSB and LSB however I never come across
 * a vendor that implements both.
 */
struct midi_ump Midi1ModWheel(uint8_t channel, uint8_t val)
{
	return Midi1ControlChange(channel, CTL_MSB_MODWHEEL, val);
}


/**
 * 14  bit value.
 * The MIDI2.0 spec says the P1 should be LSB and P2 should be MSB.
 * when encapsulating MIDI1.0 into a UMP.
 */
struct midi_ump Midi1ModWheelLSB(uint8_t channel, uint8_t val)
{
	return Midi1ControlChange(channel, CTL_LSB_MODWHEEL, val);
}


/**
 * 14 bit value 16384 = max, 8192 == dead centre. 0 is minimal. 
 * The MIDI2.0 spec says the P1 should be LSB and P2 should be MSB.
 * when encapsulating MIDI1.0 into a UMP.
 */
struct midi_ump Midi1PitchWheel(uint8_t channel, uint16_t val)
{
	return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
								   (C_PITCH_WHEEL >> 4),
								   channel & 0x0F,
								   val & MIDI_DATA,
								   (val>>7) & MIDI_DATA);
}


/**
 * -- == System realtime messages == --
 */
/* Timing Clock */
struct midi_ump Midi1TimingClock(void)
{
	return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP,RT_TIMING_CLOCK,0,0);
}


/* Start */
struct midi_ump Midi1Start(void)
{
	return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_START, 0, 0);
}


/* Continue */
struct midi_ump Midi1Continue(void)
{
	return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_CONTINUE, 0, 0);
}


/* Stop */
struct midi_ump Midi1Stop(void)
{
	return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_STOP, 0, 0);
}



/**
 * -- == System Common Messages == --
 */

/* Active Sensing */
struct midi_ump Midi1ActiveSensing(void)
{
	return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_ACTIVE_SENSING, 0, 0);
}


/* Reset */
struct midi_ump Midi1Reset(void)
{
	return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_RESET, 0, 0);
}

/* EOF */

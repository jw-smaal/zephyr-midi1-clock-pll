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
struct midi_ump midi1_note_on(uint8_t channel, uint8_t key, uint8_t velocity)
{
        return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
                                       UMP_MIDI_NOTE_ON,
                                       channel & 0x0F,
                                       key & MIDI_DATA, velocity & MIDI_DATA);
}

struct midi_ump midi1_note_off(uint8_t channel, uint8_t key, uint8_t velocity)
{
        return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
                                       UMP_MIDI_NOTE_OFF,
                                       channel & 0x0F,
                                       key & MIDI_DATA, velocity & MIDI_DATA);
}

struct midi_ump midi1_controlchange(uint8_t channel,
                                    uint8_t controller, uint8_t val)
{
        return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
                                       UMP_MIDI_CONTROL_CHANGE,
                                       channel & 0x0F,
                                       controller & MIDI_DATA, val & MIDI_DATA);
}

/*
 * Channel aftertouch is not a control change!!! 
 */
struct midi_ump midi1_channelaftertouch(uint8_t channel, uint8_t val)
{
        return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
                                       UMP_MIDI_CHAN_AFTERTOUCH,
                                       channel & 0x0F, val & MIDI_DATA, 0);
}

/*
 * Even though most keybeds don't send it; a lot of synths
 * can respond to polyphonic aftertouch.
 */
struct midi_ump midi1_polyaftertouch(uint8_t channel, uint8_t key, uint8_t val)
{
        return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
                                       UMP_MIDI_AFTERTOUCH,
                                       channel & 0x0F,
                                       key & MIDI_DATA, val & MIDI_DATA);
}

/**
 * Mod wheel has both MSB and LSB however I never come across
 * a vendor that implements both.
 */
struct midi_ump midi1_modwheel(uint8_t channel, uint8_t val)
{
        return midi1_controlchange(channel, CTL_MSB_MODWHEEL, val);
}

/**
 * 14  bit value.
 * The MIDI2.0 spec says the P1 should be LSB and P2 should be MSB.
 * when encapsulating MIDI1.0 into a UMP.
 */
struct midi_ump midi1_modwheellsb(uint8_t channel, uint8_t val)
{
        return midi1_controlchange(channel, CTL_LSB_MODWHEEL, val);
}

/**
 * 14 bit value 16384 = max, 8192 == dead centre. 0 is minimal. 
 * The MIDI2.0 spec says the P1 should be LSB and P2 should be MSB.
 * when encapsulating MIDI1.0 into a UMP.
 */
struct midi_ump midi1_pitchwheel(uint8_t channel, uint16_t val)
{
        return UMP_MIDI1_CHANNEL_VOICE(UMP_CHANNEL_GROUP,
                                       UMP_MIDI_PITCH_BEND,
                                       channel & 0x0F,
                                       val & MIDI_DATA, (val >> 7) & MIDI_DATA);
}

/**
 * -- == System realtime messages == --
 */
/* Timing Clock */
struct midi_ump midi1_timing_clock(void)
{
        return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_TIMING_CLOCK, 0, 0);
}

/* Start */
struct midi_ump midi1_start(void)
{
        return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_START, 0, 0);
}

/* Continue */
struct midi_ump midi1_continue(void)
{
        return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_CONTINUE, 0, 0);
}

/* Stop */
struct midi_ump midi1_stop(void)
{
        return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_STOP, 0, 0);
}

/**
 * -- == System Common Messages == --
 */

/* Active Sensing */
struct midi_ump midi1_activesensing(void)
{
        return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_ACTIVE_SENSING, 0, 0);
}

/* Reset */
struct midi_ump midi1_reset(void)
{
        return UMP_SYS_RT_COMMON(UMP_CHANNEL_GROUP, RT_RESET, 0, 0);
}

/*
 *------------------------------------------------------------------------------
 * MIDI tempo helpers.
 *
 * upscaled implementation (s)bpm given
 * 1.00 bpm is 100
 * 123.10 bpm is 123100 max 65535 == 655.35 bpm
 * period returned is in microseconds as a uint32_t
 * 0.003814755 3.814 ms --> 3814 us (655,35 bpm)
 * 2.500000000 s/(1/24 qn) --> 2500000 us
 * i.e. multiplied by 1000000
 * Done so we can run it on a ARM M0+ without a FPU and the need
 * to compile in single precision math.
 * By Jan-Willem Smaal <usenet@gispen.org> 20251214
 */
#define BPM_SCALE      100u
#define US_PER_SECOND  1000000u

uint32_t sbpm_to_us_interval(uint16_t sbpm)
{
        if (sbpm == 0) {
                return 0u;
        } else {
                uint64_t numer = (uint64_t) US_PER_SECOND * 60u * BPM_SCALE;
                uint64_t res = numer / (uint64_t) sbpm;
                return (uint32_t) res;
        }
}

uint16_t us_interval_to_sbpm(uint32_t interval)
{
        if (interval == 0) {
                return 0u;
        } else {
                uint64_t numer = (uint64_t) US_PER_SECOND * 60u * BPM_SCALE;
                uint64_t res = numer / (uint64_t) interval;
                return (uint32_t) res;
        }
}

uint32_t us_interval_to_24pqn(uint32_t interval)
{
        if (interval == 0) {
                return 0u;
        } else {
                return interval / 24;
        }
}

uint32_t pqn24_to_us_interval(uint32_t pqn24)
{
        if (pqn24 == 0) {
                return 0u;
        } else {
                return pqn24 * 24;
        }
}

uint32_t sbpm_to_24pqn(uint16_t sbpm)
{
        if (sbpm == 0) {
                return 0u;
        } else {
                return us_interval_to_24pqn(sbpm_to_us_interval(sbpm));
        }
}

/* -------------------------------------------------------------------------- */

/* EOF */

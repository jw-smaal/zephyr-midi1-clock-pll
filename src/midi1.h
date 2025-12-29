/**
 * midi1.h  
 *
 * Created in 2014 for ATMEL AVR MCU's ported
 * to Zephyr RTOS in 2024 mainly for ARM based MCU's. 
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @updated 20241224
 * @updated 20252810  -> for Zephyr use with USB. 
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_H
#define MIDI1_H
/*-----------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <zephyr/audio/midi.h>

/*-----------------------------------------------------------------------*/
/*  Defines specific for the MIDI protocol */
#define PITCHWHEEL_CENTER 8192

/* MIDI channel/mode masks */
#define CHANNEL_VOICE_MASK      0x80	//  Bit 7 == 1
#define CHANNEL_MODE_MASK       0xB0
#define SYSTEM_EXCLUSIVE_MASK   0xF0
#define SYSTEM_REALTIME_MASK    0XF8
#define SYSTEM_COMMON_MASK      0XF0
#define MIDI_DATA               0x7F	//  Bit 7 == 0

/* System exclusive */
#define SYSTEM_EXCLUSIVE_START  0xF0
#define SYSTEM_TUNE_REQUEST     0xF6
#define SYSTEM_EXCLUSIVE_END    0xF7

/* MIDI channel commands */
#define C_NOTE_ON               0x90
#define C_NOTE_OFF              0x80
#define C_POLYPHONIC_AFTERTOUCH 0xA0
#define C_CHANNEL_AFTERTOUCH    0xD0
#define C_PITCH_WHEEL           0xE0
#define C_CONTROL_CHANGE        0xB0
#define C_PROGRAM_CHANGE        0xC0

enum midi_control_change {
	CTL_MSB_BANK = 0x00,	// Bank Selection
	CTL_MSB_MODWHEEL = 0x01,	// Modulation
	CTL_MSB_BREATH = 0x02,	// Breath
	CTL_MSB_FOOT = 0x04,	// Foot
	CTL_MSB_PORTAMENTO_TIME = 0x05,	// Portamento Time
	CTL_MSB_DATA_ENTRY = 0x06,	// Data Entry
	CTL_MSB_MAIN_VOLUME = 0x07,	// Main Volume
	CTL_MSB_BALANCE = 0x08,	// Balance
	CTL_MSB_PAN = 0x0A,	// Panpot
	CTL_MSB_EXPRESSION = 0x0B,	// Expression
	CTL_MSB_EFFECT1 = 0x0C,	// Effect1
	CTL_MSB_EFFECT2 = 0x0D,	// Effect2
	CTL_MSB_GENERAL_PURPOSE1 = 0x10,	// General Purpose 1
	CTL_MSB_GENERAL_PURPOSE2 = 0x11,	// General Purpose 2
	CTL_MSB_GENERAL_PURPOSE3 = 0x12,	// General Purpose 3
	CTL_MSB_GENERAL_PURPOSE4 = 0x13,	// General Purpose 4
	CTL_LSB_BANK = 0x20,	// Bank Selection
	CTL_LSB_MODWHEEL = 0x21,	// Modulation
	CTL_LSB_BREATH = 0x22,	// Breath
	CTL_LSB_FOOT = 0x24,	// Foot
	CTL_LSB_PORTAMENTO_TIME = 0x25,	// Portamento Time
	CTL_LSB_DATA_ENTRY = 0x26,	// Data Entry
	CTL_LSB_MAIN_VOLUME = 0x27,	// Main Volume
	CTL_LSB_BALANCE = 0x28,	// Balance
	CTL_LSB_PAN = 0x2A,	// Panpot
	CTL_LSB_EXPRESSION = 0x2B,	// Expression
	CTL_LSB_EFFECT1 = 0x2C,	// Effect1
	CTL_LSB_EFFECT2 = 0x2D,	// Effect2
	CTL_LSB_GENERAL_PURPOSE1 = 0x30,	// General Purpose 1
	CTL_LSB_GENERAL_PURPOSE2 = 0x31,	// General Purpose 2
	CTL_LSB_GENERAL_PURPOSE3 = 0x32,	// General Purpose 3
	CTL_LSB_GENERAL_PURPOSE4 = 0x33,	// General Purpose 4
	CTL_SUSTAIN = 0x40,	// Sustain Pedal
	CTL_PORTAMENTO = 0x41,	// Portamento
	CTL_SOSTENUTO = 0x42,	// Sostenuto
	CTL_SOFT_PEDAL = 0x43,	// Soft Pedal
	CTL_LEGATO_FOOTSWITCH = 0x44,	// Legato Foot Switch
	CTL_HOLD2 = 0x45,	// Hold2
	CTL_SC1_SOUND_VARIATION = 0x46,	// SC1 Sound Variation
	CTL_SC2_TIMBRE = 0x47,	// SC2 Timbre
	CTL_SC3_RELEASE_TIME = 0x48,	// SC3 Release Time
	CTL_SC4_ATTACK_TIME = 0x49,	// SC4 Attack Time
	CTL_SC5_BRIGHTNESS = 0x4A,	// SC5 Brightness
	CTL_SC6 = 0x4B,		// SC6
	CTL_SC7 = 0x4C,		// SC7
	CTL_SC8 = 0x4D,		// SC8
	CTL_SC9 = 0x4E,		// SC9
	CTL_SC10 = 0x4F,	// SC10
	CTL_GENERAL_PURPOSE5 = 0x50,	// General Purpose 5
	CTL_GENERAL_PURPOSE6 = 0x51,	// General Purpose 6
	CTL_GENERAL_PURPOSE7 = 0x52,	// General Purpose 7
	CTL_GENERAL_PURPOSE8 = 0x53,	// General Purpose 8
	CTL_PORTAMENTO_CONTROL = 0x54,	// Portamento Control
	CTL_E1_REVERB_DEPTH = 0x5B,	// E1 Reverb Depth
	CTL_E2_TREMOLO_DEPTH = 0x5C,	// E2 Tremolo Depth
	CTL_E3_CHORUS_DEPTH = 0x5D,	// E3 Chorus Depth
	CTL_E4_DETUNE_DEPTH = 0x5E,	// E4 Detune Depth
	CTL_E5_PHASER_DEPTH = 0x5F,	// E5 Phaser Depth
	CTL_DATA_INCREMENT = 0x60,	// Data Increment
	CTL_DATA_DECREMENT = 0x61,	// Data Decrement
	CTL_NRPN_LSB = 0x62,	// Non-registered Parameter Number
	CTL_NRPN_MSB = 0x63,	// Non-registered Parameter Number
	CTL_RPN_LSB = 0x64,	// Registered Parameter Number
	CTL_RPN_MSB = 0x65,	// Registered Parameter Number
	CTL_ALL_SOUNDS_OFF = 0x78,	// All Sounds Off
	CTL_RESET_CONTROLLERS = 0x79,	// Reset Controllers
	CTL_LOCAL_CONTROL_SWITCH = 0x7A,	// Local Control Switch
	CTL_ALL_NOTES_OFF = 0x7B,	// All Notes Off
	CTL_OMNI_OFF = 0x7C,	// Omni Off
	CTL_OMNI_ON = 0x7D,	// Omni On
	CTL_MONO1 = 0x7E,	// Mono1
	CTL_MONO2 = 0x7F	// Mono2
};

/* System Real Time commands */
#define RT_TIMING_CLOCK         0xF8
#define RT_START                0xFA
#define RT_CONTINUE             0xFB
#define RT_STOP                 0xFC
#define RT_ACTIVE_SENSING       0xFE
#define RT_RESET                0xFF

/*
 * TODO: Maybe change this later to be an extra argument to the
 * TODO: functions.  For now assume UMP channel group = 0.
 */
#define UMP_CHANNEL_GROUP 0

/*
 * Global variables
 * TODO: these only make sense when using USART/UART maybe need
 * TODO: implement when creating a UMP bridge. 
 */
//static uint8_t global_running_status_tx;
//static uint_t global_running_status_rx;

/**
 * -- == Channel messages == --
 */
struct midi_ump midi1_note_on(uint8_t channel, uint8_t key, uint8_t velocity);
struct midi_ump midi1_note_off(uint8_t channel, uint8_t key, uint8_t velocity);
struct midi_ump midi1_controlchange(uint8_t channel,
				    uint8_t controller, uint8_t val);
struct midi_ump midi1_pitchwheel(uint8_t channel, uint16_t val);
struct midi_ump midi1_modwheel(uint8_t channel, uint8_t val);
struct midi_ump midi1_polyaftertouch(uint8_t channel, uint8_t key, uint8_t val);
struct midi_ump midi1_channelaftertouch(uint8_t channel, uint8_t val);

/**
 * -- == System realtime messages == --
 */
struct midi_ump midi1_timing_clock(void);
struct midi_ump midi1_start(void);
struct midi_ump midi1_continue(void);
struct midi_ump midi1_stop(void);
struct midi_ump midi1_active_sensing(void);
struct midi_ump midi1_reset(void);


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
 *
 */
#define BPM_SCALE      100u
#define US_PER_SECOND  1000000u

/**
 * @brief returns the interval in microseconds (us) for a given sbpm
 * @param  sbpm   Scaled BPM value (e.g. 12000 for 120.00 BPM)
 * @return interval in microseconds (us)
 */
uint32_t sbpm_to_us_interval(uint16_t sbpm);

/**
 * @brief returns the interval in clock ticks for a given sbpm
 * @param sbpm   Scaled BPM value (e.g. 12000 for 120.00 BPM)
 * @param clock_hz clock speed of the current processor
 * @return interval clock ticks
 */
uint32_t sbpm_to_ticks(uint16_t sbpm, uint32_t clock_hz);

/**
 * @brief Convert a measured interval in microseconds to scaled BPM (sbpm).
 *
 * @param interval  Interval duration in microseconds (us)
 * @return Scaled BPM value (e.g. 12000 for 120.00 BPM)
 */
uint16_t us_interval_to_sbpm(uint32_t interval);

/**
 * @brief Convert a measured interval in microseconds to 24‑PPQN period units.
 *
 * @param interval  Interval duration in microseconds (us)
 * @return 24‑PPQN period value corresponding to the interval
 */
uint32_t us_interval_to_24pqn(uint32_t interval);

/**
 * @brief Convert a 24‑PPQN period value to an interval in microseconds.
 *
 * @param pqn24  24‑PPQN period value
 * @return Interval duration in microseconds (us)
 */
uint32_t pqn24_to_us_interval(uint32_t pqn24);

/**
 * @brief Convert scaled BPM (sbpm) to a 24‑PPQN period value.
 *
 * @param sbpm  Scaled BPM value (e.g. 12000 for 120.00 BPM)
 * @return 24‑PPQN period value corresponding to the BPM
 */
uint32_t sbpm_to_24pqn(uint16_t sbpm);

/**
 * @brief  Returns static string with the BPM formattted like 123.45
 *
 * @param  sbpm   Scaled BPM value (e.g. 12000 for 120.00 BPM)
 * @return Pointer to a static buffer containing the formatted string "xxx.yy"
 */
const char *sbpm_to_str(uint16_t sbpm);

/* -------------------------------------------------------------------------- */
#endif
/* EOF */

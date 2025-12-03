/**
 * serial-usart-midi.h
 *
 * Created in 2014 ported to Zephyr RTOS in 2024. 
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef _SERIAL_USART_MIDI
#define _SERIAL_USART_MIDI
/*-----------------------------------------------------------------------*/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <stdint.h>
#include <string.h>

#define MIDI_DEBUG 0

/*-----------------------------------------------------------------------*/
/*  Defines specific for the MIDI protocol */ 
#define PITCHWHEEL_CENTER 8192

/* MIDI channel/mode masks */
#define CHANNEL_VOICE_MASK      0x80    //  Bit 7 == 1
#define CHANNEL_MODE_MASK       0xB0
#define SYSTEM_EXCLUSIVE_MASK   0xF0
#define SYSTEM_REALTIME_MASK    0XF8
#define SYSTEM_COMMON_MASK      0XF0
#define MIDI_DATA               0x7F    //  Bit 7 == 0

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
    CTL_MSB_BANK             = 0x00,  // Bank Selection
    CTL_MSB_MODWHEEL         = 0x01,  // Modulation
    CTL_MSB_BREATH           = 0x02,  // Breath
    CTL_MSB_FOOT             = 0x04,  // Foot
    CTL_MSB_PORTAMENTO_TIME  = 0x05,  // Portamento Time
    CTL_MSB_DATA_ENTRY       = 0x06,  // Data Entry
    CTL_MSB_MAIN_VOLUME      = 0x07,  // Main Volume
    CTL_MSB_BALANCE          = 0x08,  // Balance
    CTL_MSB_PAN              = 0x0A,  // Panpot
    CTL_MSB_EXPRESSION       = 0x0B,  // Expression
    CTL_MSB_EFFECT1          = 0x0C,  // Effect1
    CTL_MSB_EFFECT2          = 0x0D,  // Effect2
    CTL_MSB_GENERAL_PURPOSE1 = 0x10,  // General Purpose 1
    CTL_MSB_GENERAL_PURPOSE2 = 0x11,  // General Purpose 2
    CTL_MSB_GENERAL_PURPOSE3 = 0x12,  // General Purpose 3
    CTL_MSB_GENERAL_PURPOSE4 = 0x13,  // General Purpose 4
    CTL_LSB_BANK             = 0x20,  // Bank Selection
    CTL_LSB_MODWHEEL         = 0x21,  // Modulation
    CTL_LSB_BREATH           = 0x22,  // Breath
    CTL_LSB_FOOT             = 0x24,  // Foot
    CTL_LSB_PORTAMENTO_TIME  = 0x25,  // Portamento Time
    CTL_LSB_DATA_ENTRY       = 0x26,  // Data Entry
    CTL_LSB_MAIN_VOLUME      = 0x27,  // Main Volume
    CTL_LSB_BALANCE          = 0x28,  // Balance
    CTL_LSB_PAN              = 0x2A,  // Panpot
    CTL_LSB_EXPRESSION       = 0x2B,  // Expression
    CTL_LSB_EFFECT1          = 0x2C,  // Effect1
    CTL_LSB_EFFECT2          = 0x2D,  // Effect2
    CTL_LSB_GENERAL_PURPOSE1 = 0x30,  // General Purpose 1
    CTL_LSB_GENERAL_PURPOSE2 = 0x31,  // General Purpose 2
    CTL_LSB_GENERAL_PURPOSE3 = 0x32,  // General Purpose 3
    CTL_LSB_GENERAL_PURPOSE4 = 0x33,  // General Purpose 4
    CTL_SUSTAIN              = 0x40,  // Sustain Pedal
    CTL_PORTAMENTO           = 0x41,  // Portamento
    CTL_SOSTENUTO            = 0x42,  // Sostenuto
    CTL_SOFT_PEDAL           = 0x43,  // Soft Pedal
    CTL_LEGATO_FOOTSWITCH    = 0x44,  // Legato Foot Switch
    CTL_HOLD2                = 0x45,  // Hold2
    CTL_SC1_SOUND_VARIATION  = 0x46,  // SC1 Sound Variation
    CTL_SC2_TIMBRE           = 0x47,  // SC2 Timbre
    CTL_SC3_RELEASE_TIME     = 0x48,  // SC3 Release Time
    CTL_SC4_ATTACK_TIME      = 0x49,  // SC4 Attack Time
    CTL_SC5_BRIGHTNESS       = 0x4A,  // SC5 Brightness
    CTL_SC6                  = 0x4B,  // SC6
    CTL_SC7                  = 0x4C,  // SC7
    CTL_SC8                  = 0x4D,  // SC8
    CTL_SC9                  = 0x4E,  // SC9
    CTL_SC10                 = 0x4F,  // SC10
    CTL_GENERAL_PURPOSE5     = 0x50,  // General Purpose 5
    CTL_GENERAL_PURPOSE6     = 0x51,  // General Purpose 6
    CTL_GENERAL_PURPOSE7     = 0x52,  // General Purpose 7
    CTL_GENERAL_PURPOSE8     = 0x53,  // General Purpose 8
    CTL_PORTAMENTO_CONTROL   = 0x54,  // Portamento Control
    CTL_E1_REVERB_DEPTH      = 0x5B,  // E1 Reverb Depth
    CTL_E2_TREMOLO_DEPTH     = 0x5C,  // E2 Tremolo Depth
    CTL_E3_CHORUS_DEPTH      = 0x5D,  // E3 Chorus Depth
    CTL_E4_DETUNE_DEPTH      = 0x5E,  // E4 Detune Depth
    CTL_E5_PHASER_DEPTH      = 0x5F,  // E5 Phaser Depth
    CTL_DATA_INCREMENT       = 0x60,  // Data Increment
    CTL_DATA_DECREMENT       = 0x61,  // Data Decrement
    CTL_NRPN_LSB             = 0x62,  // Non-registered Parameter Number
    CTL_NRPN_MSB             = 0x63,  // Non-registered Parameter Number
    CTL_RPN_LSB              = 0x64,  // Registered Parameter Number
    CTL_RPN_MSB              = 0x65,  // Registered Parameter Number
    CTL_ALL_SOUNDS_OFF       = 0x78,  // All Sounds Off
    CTL_RESET_CONTROLLERS    = 0x79,  // Reset Controllers
    CTL_LOCAL_CONTROL_SWITCH = 0x7A,  // Local Control Switch
    CTL_ALL_NOTES_OFF        = 0x7B,  // All Notes Off
    CTL_OMNI_OFF             = 0x7C,  // Omni Off
    CTL_OMNI_ON              = 0x7D,  // Omni On
    CTL_MONO1                = 0x7E,  // Mono1
    CTL_MONO2                = 0x7F   // Mono2
};

/* System Real Time commands */
#define RT_TIMING_CLOCK         0xF8
#define RT_START                0xFA
#define RT_CONTINUE             0xFB
#define RT_STOP                 0xFC
#define RT_ACTIVE_SENSING       0xFE
#define RT_RESET                0xFF

/*-----------------------------------------------------------------------*/
/* Global variables */
static uint8_t global_running_status_tx;
static uint8_t global_running_status_rx;
static uint8_t global_3rd_byte_flag;
static uint8_t global_midi_c2;
static uint8_t global_midi_c3;

/*-----------------------------------------------------------------------*/
/*  Function prototypes */
void SerialMidiReceiveParser(void);

/* During the SerialMidiInit the delegate callback functions need to be assigned */
void SerialMidiInit(void (*note_on_handler_ptr)(uint8_t note, uint8_t velocity),
                    void (*note_off_handler_ptr)(uint8_t note, uint8_t velocity),
                    void (*control_change_handler_ptr)(uint8_t controller, uint8_t value),
                    void (*realtime_handler_ptr)(uint8_t msg),
                    void (*midi_pitchwheel_ptr)(uint8_t lsb, uint8_t msb));

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

/*  "private" functions used for the implementation. */
void serial_isr_callback(const struct device *dev, void *user_data);
int midi_fifo_get(uint8_t *data);
int midi_msgq_get(uint8_t *data);


#endif /* _SERIAL_USART_MIDI */
/* EOF */
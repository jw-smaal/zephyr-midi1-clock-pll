/**
 *  note.h 
 * 
 * @brief generic MIDI and harmony related functions 
 * implemented in support of embedded systems in c. 
 * tested on Zephyr RTOS.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef HARMONY_NOTE_H
#define HARMONY_NOTE_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*
 * Pre computed values for the note frequencies
 */
#include "midi_freq_table.h"

/* 440 Hz for the A4 note */
#define BASE_A4_NOTE_FREQUENCY 440

const char *noteToText(uint8_t midinote, bool flats);
const char *noteToTextWithOctave(uint8_t midinote, bool flats);
int noteToOct(uint8_t midinote);
#if TODO_USING_MATH
float noteToFreq(uint8_t midinote, int base_a4_note_freq);
#endif
float noteToFreq(uint8_t midinote);
uint8_t freqToMidiNote(float freq);

#endif				/* HARMONY_NOTE_H */
/* EOF */

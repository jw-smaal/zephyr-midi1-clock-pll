/** 
 * note.c 
 * 
 * @brief Generic MIDI and harmony related functions 
 * implemented in support of embedded systems in c. 
 * tested on Zephyr RTOS. 
 * 
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include "note.h"


/* Return the note with the octave included. */
char *noteToTextWithOctave(uint8_t midinote, bool flats) {
    static char notestring[4];
    snprintf(notestring,
             sizeof(notestring),
            "%s%d", 
            noteToText(midinote, flats), noteToOct(midinote));

    return &notestring[0];  
}


/* This function converts a MIDI note using a lookup table to a string */
char *noteToText(uint8_t midinote, bool flats) {
    int octave = noteToOct(midinote);
    uint8_t note = midinote - ((octave + 2) * 12);
    static char *flatNotes[] =  { "C ", "Db", "D ", "Eb", "E ", "F ", "Gb", "G ", "Ab", "A ", "Bb", "B "};
    static char *sharpNotes[] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};

    char *noteString;
    if (flats) {
        noteString = flatNotes[note];
    } else {
        noteString = sharpNotes[note];
    }
    /*  Null-terminated string is returned from the static char array */
    return noteString; 
}


/*
 * This function converts a MIDI note to an octave number 
 */
int noteToOct(uint8_t midinote) {
    return (midinote / 12) - 2;
}


#if 0
/*
 * This function converts a MIDI note to a frequency 
 */ 
float noteToFreq(uint8_t midinote, int base_a4_note_freq) {
    return base_a4_note_freq * pow(2, (midinote - 69) / 12.0);
}

#endif 

/*
 * This function converts a frequency to a MIDI note 
 * this will be slow as we have to iterate over all 128 notes.
 * There are faster ways to do this. 
 */
uint8_t freqToMidiNoteSlow(float freq) {
    uint8_t midinote = 0;

    for (int i = 0; i < 128; i++) {
        if (freq >= noteToFreq(i, BASE_A4_NOTE_FREQUENCY)) {
            midinote = i;
        } else {
            break;
        }
    }
    return midinote;
}


/* 
 * This is a binary search version of the frequency to MIDI note conversion.
 */
uint8_t freqToMidiNoteFaster(float freq) {
    /* Perform a binary search on the MIDI notes */
    int min = 0;
    int max = 127;
    while (min <= max) {
        int mid = (min + max) / 2;
        if (freq == noteToFreq(mid, BASE_A4_NOTE_FREQUENCY)) {
            return mid;
        } else if (freq < noteToFreq(mid, BASE_A4_NOTE_FREQUENCY)) {
            max = mid - 1;
        } else {
            min = mid + 1;
        }
    }
    return 0; /* No match found, return the first MIDI note (C-1) */
}


/* 
 * Use a binary search algorithm to find the MIDI note corresponding 
 * to the given frequency value. Start by setting the minimum and maximum values of the search range to 
 * the first and last MIDI notes, respectively. Then, in each iteration of the while loop, calculate the 
 * midpoint between the minimum and maximum values, and compare it to the given frequency value. If the 
 * frequency value is equal or greater than the midpoint, set the minimum value to the midpoint plus one, 
 * and repeat the search. If the frequency value is less than the midpoint, set the maximum value to the 
 * midpoint minus one, and repeat the search. In the end return the minimum value which corresponds to 
 * the MIDI note that best matches the given frequency value.
 */
uint8_t freqToMidiNote(float freq) {
    /* Use a binary search to find the MIDI note corresponding to the given frequency value */
    int min = 0;
    int max = 127;
    while (min < max) {
        int mid = (min + max) / 2;
        if (freq >= noteToFreq(mid, BASE_A4_NOTE_FREQUENCY)) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    }

    /* Return the MIDI note corresponding to the given frequency value */
    return min; 
}

/* EOF */

#ifndef MIDI1_BLOCKAVG_H
#define MIDI1_BLOCKAVG_H
/**
 * @file midi1_blockavg.h
 * @brief average the BPM measurement samples
 * @note used by the midi_clock_measure_counter.c and .h
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20260102
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

/**
 * @note over which amount of samples do we take the
 * measurement.
 */
#define MIDI1_BLOCKAVG_SIZE 64

/**
 * @brief init the block average system
 */
void midi1_blockavg_init(void);

/**
 * @brief add current sample
 */
void midi1_blockavg_add(uint32_t sample);

/**
 * @brief get average of current block
 * @return block average
 */
uint32_t midi1_blockavg_average(void);

/**
 * @brief getter for the current sample
 * @return count number the sample
 */
uint32_t midi1_blockavg_count(void);

#endif
/* EOF */

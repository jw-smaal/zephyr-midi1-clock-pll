/**
 * @file midi1_clock_pll.h
 * @brief Simple integer PLL for MIDI clock synchronization (24 PPQN).
 * @author Jan-Willem Smaal
 * @date 20251229
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_CLOCK_PLL_H
#define MIDI1_CLOCK_PLL_H

#include <stdint.h>

/**
 * @brief Initialize the MIDI1 PLL with a nominal BPM.
 *
 * @param sbpm  Scaled BPM value (e.g. 12000 for 120.00 BPM)
 */
void midi1_pll_init(uint16_t sbpm);

/**
 * @brief Process an incoming MIDI clock tick timestamp.
 *
 * @param t_in_us  Timestamp of the external tick in microseconds.
 */
void midi1_pll_process_tick(uint32_t t_in_us);

/**
 * @brief Get the current PLL‑corrected 24pqn interval in microseconds.
 *
 * @return Interval in microseconds for the next 24pqn internal tick.
 */
int32_t midi1_pll_get_interval_us(void);

#endif				/* MIDI1_PLL_H */

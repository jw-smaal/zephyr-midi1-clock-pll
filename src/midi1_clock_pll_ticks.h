/**
 * @file midi1_clock_pll_ticks.h
 * @brief Simple integer PLL for MIDI clock synchronization (24 PPQN).
 * @author Jan-Willem Smaal
 * @date 20251229
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_CLOCK_PLL_TICKS_H
#define MIDI1_CLOCK_PLL_TICKS_H
#include <stdint.h>

/**
 * @brief Initialize the MIDI1 PLL with a nominal BPM.
 *
 * @param sbpm  Scaled BPM value (e.g. 12000 for 120.00 BPM)
 */
void midi1_pll_ticks_init(uint16_t sbpm);

/**
 * @brief Process an incoming MIDI clock tick interval.
 *
 * @param measured_interval_ticks interval in ticks.
 */
void midi1_pll_ticks_process_interval(uint32_t measured_interval_ticks);

/**
 * @brief Get the current PLL‑corrected 24pqn interval in microseconds.
 *
 * @return Interval in microseconds for the next 24pqn internal tick.
 */
//int32_t midi1_pll_ticks_get_interval_us(void);

int32_t midi1_pll_ticks_get_interval_ticks(void);

#endif	/* MIDI1_CLOCK_PLL_TICKS_H */

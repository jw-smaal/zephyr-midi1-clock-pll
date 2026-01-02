#ifndef MIDI1_BLOCKAVG_H
#define MIDI1_BLOCKAVG_H

#include <stdint.h>

#define MIDI1_BLOCKAVG_SIZE 64

void midi1_blockavg_init(void);
void midi1_blockavg_add(uint32_t sample);
uint32_t midi1_blockavg_average(void);
uint32_t midi1_blockavg_count(void);

#endif

#include <stdint.h>
#include "midi1_blockavg.h"

static uint32_t buf[MIDI1_BLOCKAVG_SIZE];
static uint32_t sum = 0;
static uint32_t index = 0;
static uint32_t count = 0;

void midi1_blockavg_init(void)
{
	for (uint32_t i = 0; i < MIDI1_BLOCKAVG_SIZE; i++) {
		buf[i] = 0;
	}
	sum = 0;
	index = 0;
	count = 0;
}

void midi1_blockavg_add(uint32_t sample)
{
	if (count < MIDI1_BLOCKAVG_SIZE) {
		/* Still filling the buffer */
		buf[count] = sample;
		sum += sample;
		count++;
	} else {
		/* Buffer full: overwrite oldest */
		sum -= buf[index];
		buf[index] = sample;
		sum += sample;

		index++;
		if (index >= MIDI1_BLOCKAVG_SIZE) {
			index = 0;
		}
	}
}

uint32_t midi1_blockavg_average(void)
{
	if (count == 0) {
		return 0;
	}
	return sum / count;
}

uint32_t midi1_blockavg_count(void)
{
	return count;
}

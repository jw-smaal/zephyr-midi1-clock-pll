#ifndef MIDI1_CLOCK_TIMER
#define MIDI1_CLOCK_TIMER
/*
 * MIDI1.0 clock for zephyr RTOS
 *
 * By Jan-Willem Smaal <usenet@gispen.org> 20251214
 */
#include <zephyr/kernel.h>      /* k_timer */
#include <zephyr/sys/atomic.h>  /* atomic_t, atomic_get/set */
#include <stdint.h>             /* uint32_t, uint16_t */
#include <stddef.h>             /* NULL */

/* Timer and running flag */
static struct k_timer g_midi_timer;
static atomic_t g_midi_running = ATOMIC_INIT(0);

/* Timer handler runs in system workqueue context; keep it short */
//static void     midi_timer_handler(struct k_timer *t);

/*
 * Initialize MIDI clock subsystem with your MIDI device handle. Call once at
 * startup before starting the clock.
 */
void            midi_clock_init(void *midi_dev);

/*
 * Start periodic MIDI clock. interval_us must be > 0. Uses k_timer_start with
 * the same period for initial and repeat time.
 */
void            midi_clock_start(uint32_t interval_us);

/* Stop the clock */
void            midi_clock_stop(void);

#endif                          /* MIDI1_CLOCK_TIMER */
/* EOF */

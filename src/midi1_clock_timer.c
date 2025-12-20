/*
 * implementation of midi1_clock_timer by Jan-Willem Smaal <usenet@gispen.org
 * this is a software timer in zephyr.  
 * 20251214
 */
#include <zephyr/audio/midi.h>

/* This is part of the MIDI2 library prj.conf
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h it get's linked in.
 */
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <ump_stream_responder.h>

/* MIDI helpers by J-W Smaal*/
#include "midi1.h"
#include "midi1_clock_timer.h"

/* Timer and running flag */
static struct k_timer g_midi1_timer;
static atomic_t g_midi1_running = ATOMIC_INIT(0);

/* Timer handler runs in system workqueue context; keep it short */
/* static and kept local to the implementation */
static void midi1_timer_handler(struct k_timer *t)
{
        //void *midi_dev = k_timer_user_data_get(t);
        const struct device *midi1_dev = k_timer_user_data_get(t);

        if (!atomic_get(&g_midi1_running)) {
                return;
        }
        if (midi1_dev) {
                usbd_midi_send(midi1_dev, midi1_timing_clock());
        }
}

/*
 * Initialize MIDI clock subsystem with your MIDI device handle. Call once at
 * startup before starting the clock.
 */
void midi1_clock_init(const struct device *midi1_dev_arg)
{
        atomic_set(&g_midi1_running, 0);
        k_timer_init(&g_midi1_timer, midi1_timer_handler, NULL);
        k_timer_user_data_set(&g_midi1_timer, (void *)midi1_dev_arg);
}

/*
 * Start periodic MIDI clock. interval_us must be > 0. Uses k_timer_start with
 * the same period for initial and repeat time.
 */
void midi1_clock_start(uint32_t interval_us)
{
        if (interval_us == 0u) {
                return;
        }
        atomic_set(&g_midi1_running, 1);
        k_timer_start(&g_midi1_timer, K_USEC(interval_us), K_USEC(interval_us));
}

/* Stop the clock */
void midi1_clock_stop(void)
{
        atomic_set(&g_midi1_running, 0);
        k_timer_stop(&g_midi1_timer);
}

/* EOF */

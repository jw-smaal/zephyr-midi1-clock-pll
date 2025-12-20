/*
 * implementation of midi1_clock_clock by Jan-Willem Smaal <usenet@gispen.org
 * this is a hardware based counter tested with NXP FRDM_MCXC242 in zephyr.  
 * 20251214
 */
#include <zephyr/audio/midi.h>
#include <zephyr/drivers/counter.h>

/* This is part of the MIDI2 library prj.conf
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h it get's linked in.
 */
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <ump_stream_responder.h>

/* MIDI helpers by J-W Smaal*/
#include "midi1.h"
#include "midi1_clock_counter.h"

/* Timer and running flag */
static atomic_t g_midi1_running_cntr = ATOMIC_INIT(0);

/* static and kept local to the implementation */
static void midi1_cntr_handler(const struct *midi_dev)
{
        //void *midi_dev = k_timer_user_data_get(t);
		void *midi_dev = user_data; 
        //const struct device *midi1_dev = k_timer_user_data_get(t);

        if (!atomic_get(&g_midi1_running_cntr)) {
                return;
        }
      //  if (midi1_dev) {
      //          usbd_midi_send(midi1_dev, midi1_timing_clock());
      //  }
}

/*
 * Initialize MIDI clock subsystem with your MIDI device handle. Call once at
 * startup before starting the clock.
 */
void midi1_clock_cntr_init(const struct device *midi1_dev_arg)
{
        atomic_set(&g_midi1_running_cntr, 0);
      //  k_timer_init(&g_midi1_timer, midi1_timer_handler, NULL);
      //  k_timer_user_data_set(&g_midi1_timer, (void *)midi1_dev_arg);
}

/*
 * Start periodic MIDI clock. interval_us must be > 0. Uses k_timer_start with
 * the same period for initial and repeat time.
 */
void midi1_clock_cntr_start(uint32_t interval_us)
{
        if (interval_us == 0u) {
                return;
        }
        atomic_set(&g_midi1_running_cntr, 1);
       // k_timer_start(&g_midi1_timer, K_USEC(interval_us), K_USEC(interval_us));
}

/* Stop the clock */
void midi1_clock_cntr_stop(void)
{
        atomic_set(&g_midi1_running_cntr, 0);
        //k_timer_stop(&g_midi1_timer);
}

/* EOF */

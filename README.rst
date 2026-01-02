===========================================
Zephyr MIDI 1.0 Clock + PLL (MCX / FRDM)
===========================================

A hardware-accurate, integer-only MIDI 1.0 clock generator, PLL, and
measurement subsystem for NXP MCX microcontrollers running Zephyr RTOS.

based on usb-midi zephyr code from: 
Copyright (c) 2024 Titouan Christophe
SPDX-License-Identifier: Apache-2.0

This project provides:

* A **MIDI 1.0 clock generator** using a Zephyr ``counter`` device
* A **hardware-timestamped MIDI clock measurement module**
* A **fixed‑point PLL** for stabilizing incoming MIDI clock
* Clean, explicit, maintainable C code suitable for ARM M0+
* Verified timing on FRDM-MCXC242 using GPIO + oscilloscope

The design avoids ``k_cycle_get_32()`` and other OS‑scheduled timing
sources. All timing is derived from a **free‑running hardware counter**
for microsecond‑accurate measurement.

---------------------------------------
Features
---------------------------------------

* **24 PPQN MIDI clock generation**
* **PLL‑based MIDI clock following**
* **Hardware timestamping** of incoming MIDI Clock (0xF8)
* **Integer‑only BPM math** (no FPU required)
* **Scaled BPM representation** (e.g. 123.45 BPM → ``12345``)
* **Zephyr‑compliant device model**
* **USB‑MIDI 2.0 UMP support** (via Zephyr MIDI2 library)
* Optional **GPIO clock output** for oscilloscope verification

---------------------------------------
Hardware Requirements
---------------------------------------

Tested on:

* **NXP FRDM-MCXC242**
* Zephyr RTOS (3.6+ recommended)
* Any Zephyr-supported ``counter`` device with microsecond resolution

The design is portable to other MCX boards or any MCU with a suitable
hardware counter.

---------------------------------------
Repository Structure
---------------------------------------

::

    src/
      midi1_clock_counter.c        # MIDI clock generator (Zephyr counter)
      midi1_clock_meas_cntr.c      # Hardware-timestamped measurement
      midi1.c / midi1.h            # MIDI helpers
      main.c                       # UMP responder + integration
...

    include/
      midi1_clock_counter.h
      midi1_clock_meas_cntr.h
      midi1.h
...

---------------------------------------
MIDI Clock Generation
---------------------------------------

Clock generation uses a Zephyr ``counter`` device configured with a
periodic top value. Each overflow triggers an ISR that sends a MIDI
Clock byte (0xF8).

Example:

.. code-block:: c

   uint32_t ticks = sbpm_to_ticks(sbpm, midi1_clock_cntr_cpu_frequency());
   midi1_clock_cntr_ticks_start(ticks);

The generator is fully hardware-driven and does not rely on threads or
software timers.

---------------------------------------
MIDI Clock Measurement
---------------------------------------

Incoming MIDI Clock pulses are timestamped using a **free-running
hardware counter**:

.. code-block:: c

   void midi1_clock_meas_cntr_pulse(void)
   {
       uint32_t now_us = meas_now_us();
       ...
   }


The BPM is computed using:

.. code-block:: c

   scaledBPM = 250000000 / interval_us;

---------------------------------------
PLL (Phase-Locked Loop)
---------------------------------------

The PLL smooths incoming MIDI clock timing and produces a stable,
low-jitter internal tempo. It is designed for:

* USB MIDI clock (jittery)
* DIN MIDI clock (stable)
* Internal loopback testing

The PLL operates entirely in integer math and is safe for ARM M0+.

---------------------------------------
Building
---------------------------------------

Standard Zephyr build:

.. code-block:: sh

   west build -b frdm_mcxc242 -p always

---------------------------------------
Running
---------------------------------------

Connect the board via USB. The project exposes:

* USB-MIDI 2.0 UMP endpoint
* Optional GPIO clock output for oscilloscope verification
* PLL-stabilized internal tempo

---------------------------------------
License
---------------------------------------

Apache-2.0

---------------------------------------
Author
---------------------------------------

Jan-Willem Smaal <usenet@gispen.org>

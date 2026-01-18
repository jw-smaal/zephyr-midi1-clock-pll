===========================================
Zephyr MIDI 1.0 Clock + PLL (MCX / FRDM)
===========================================

A hardware MIDI 1.0 clock generator, PLL, and
measurement subsystem for microcontrollers running Zephyr RTOS.

based on usb-midi zephyr code from: 
Copyright (c) 2024 Titouan Christophe
SPDX-License-Identifier: Apache-2.0

This project provides:

* A **MIDI 1.0 clock generator** using a Zephyr ``counter`` device
* A **hardware-timestamped MIDI clock measurement module**
* A **fixed‑point PLL** for stabilizing incoming MIDI clock
* C code suitable for Zephyr on ARM M0+ MCU's. 
* Verified timing on NXP FRDM-MCXC242 using GPIO + oscilloscope
* Also tested on NXP FRDM-K64F and FRDM-MCXA156.   

The design avoids ``k_cycle_get_32()`` and other OS‑scheduled timing
sources. All timing is derived from a **free‑running hardware counter**
for microsecond‑accurate measurement.

[ Note to make use of the MIDI_OUT functionallity be aware most low 
 power UARTS on MCU's cannot sink the current required for the MIDI loop. 
 Use a buffer that can sink 5 mA to ground.  ]

---------------------------------------
Features
---------------------------------------

* **24 PPQN MIDI clock generation**
* **PLL‑based MIDI clock following**
* **Hardware timestamping** of incoming MIDI Clock (0xF8)
* **Integer‑only BPM math** (no FPU required)
* **Scaled BPM representation** (e.g. 123.45 BPM → ``12345``)
* **USB‑MIDI 2.0 UMP support** (via Zephyr MIDI2 library)
* Optional **GPIO clock output** for oscilloscope verification

---------------------------------------
Hardware Requirements
---------------------------------------

Tested on:

* **NXP FRDM-MCXC242**
* **NXP FRDM-K64F** 
* **NXP FRDM-MCXA156** 
* Zephyr RTOS (3.6+ recommended)
* Any Zephyr-supported ``counter`` device with microsecond resolution

The design is portable to other MCX boards or any MCU with a suitable
hardware counter.

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

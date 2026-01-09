#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <stdint.h>

#include "midi1.h"
#include "midi1_serial.h"

/* Globals to capture callback results */
static uint8_t g_last_note;
static uint8_t g_last_vel;
static uint8_t g_last_ctrl;
static uint8_t g_last_val;
static uint8_t g_last_rt;
static uint8_t g_pw_lsb;
static uint8_t g_pw_msb;

/* Test callbacks */
static void test_note_on_cb(uint8_t note, uint8_t velocity)
{
    g_last_note = note;
    g_last_vel  = velocity;
}

static void test_note_off_cb(uint8_t note, uint8_t velocity)
{
    g_last_note = note;
    g_last_vel  = velocity;
}

static void test_cc_cb(uint8_t ctrl, uint8_t val)
{
    g_last_ctrl = ctrl;
    g_last_val  = val;
}

static void test_rt_cb(uint8_t msg)
{
    g_last_rt = msg;
}

static void test_pw_cb(uint8_t lsb, uint8_t msb)
{
    g_pw_lsb = lsb;
    g_pw_msb = msb;
}

/* Helper: init a fresh instance with callbacks and msgq */
static void midi_test_inst_init(struct midi1_serial_inst *inst)
{
    memset(inst, 0, sizeof(*inst));

    inst->note_on        = test_note_on_cb;
    inst->note_off       = test_note_off_cb;
    inst->control_change = test_cc_cb;
    inst->realtime       = test_rt_cb;
    inst->pitchwheel     = test_pw_cb;

    k_msgq_init(&inst->msgq, inst->msgq_buffer, MSG_SIZE, MSGQ_SIZE);

    midi1_serial_init(inst);

    g_last_note = g_last_vel = 0;
    g_last_ctrl = g_last_val = 0;
    g_last_rt   = 0;
    g_pw_lsb    = g_pw_msb = 0;
}

/* Helper: feed bytes into msgq and run parser */
static void feed_and_parse(struct midi1_serial_inst *inst,
                           const uint8_t *data,
                           size_t len)
{
    for (size_t i = 0; i < len; i++) {
        k_msgq_put(&inst->msgq, &data[i], K_NO_WAIT);
        midi1_serial_receiveparser(inst);
    }
}

/* Basic NOTE ON test */
ZTEST(midi1_parser, test_note_on_basic)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    uint8_t seq[] = { 0x90, 60, 100 }; /* CH1, note 60, vel 100 */

    feed_and_parse(&inst, seq, sizeof(seq));

    zassert_equal(g_last_note, 60, "Expected note 60");
    zassert_equal(g_last_vel, 100, "Expected velocity 100");
}

/* NOTE ON with velocity 0 -> NOTE OFF */
ZTEST(midi1_parser, test_note_on_velocity_zero_as_off)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    uint8_t seq[] = { 0x90, 60, 0 }; /* CH1, note 60, vel 0 */

    feed_and_parse(&inst, seq, sizeof(seq));

    zassert_equal(g_last_note, 60, "Expected note 60");
    zassert_equal(g_last_vel, 0, "Expected velocity 0");
}

/* Running status for NOTE ON */
ZTEST(midi1_parser, test_running_status_note_on)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    uint8_t seq[] = {
        0x90, 60, 100,  /* status + 2 data */
        62, 110,        /* running status NOTE ON */
        64, 120         /* running status NOTE ON */
    };

    feed_and_parse(&inst, seq, sizeof(seq));

    zassert_equal(g_last_note, 64, "Expected last note 64");
    zassert_equal(g_last_vel, 120, "Expected last velocity 120");
}

/* Control Change with running status */
ZTEST(midi1_parser, test_control_change_running_status)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    uint8_t seq[] = {
        0xB0, 1, 64,    /* CC1 = 64 */
        1, 127          /* running status CC1 = 127 */
    };

    feed_and_parse(&inst, seq, sizeof(seq));

    zassert_equal(g_last_ctrl, 1, "Expected controller 1");
    zassert_equal(g_last_val, 127, "Expected value 127");
}

/* Realtime interleaving should not break running status */
ZTEST(midi1_parser, test_realtime_interleave)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    uint8_t seq[] = {
        0x90, 60, 100,  /* NOTE ON */
        0xF8,           /* TIMING CLOCK (realtime) */
        62, 110         /* running status NOTE ON */
    };

    feed_and_parse(&inst, seq, sizeof(seq));

    zassert_equal(g_last_rt, 0xF8, "Expected realtime 0xF8");
    zassert_equal(g_last_note, 62, "Expected note 62");
    zassert_equal(g_last_vel, 110, "Expected velocity 110");
}

/* Pitch wheel test */
ZTEST(midi1_parser, test_pitchwheel)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    uint8_t seq[] = {
        0xE0, 0x00, 0x40  /* center position: LSB=0, MSB=64 */
    };

    feed_and_parse(&inst, seq, sizeof(seq));

    zassert_equal(g_pw_lsb, 0x00, "Expected LSB 0x00");
    zassert_equal(g_pw_msb, 0x40, "Expected MSB 0x40");
}

/* Data byte with no running status should be ignored */
ZTEST(midi1_parser, test_data_without_running_status_ignored)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    uint8_t seq[] = { 0x40, 0x41, 0x42 }; /* all data bytes, no status */

    feed_and_parse(&inst, seq, sizeof(seq));

    zassert_equal(g_last_note, 0, "Expected no note callback");
    zassert_equal(g_last_vel, 0, "Expected no velocity callback");
}

ZTEST(midi1_parser, test_running_status_modwheel_sweep)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    /* First message sets running status: CC1 = 0 */
    uint8_t first[] = { 0xB0, 1, 0 };  /* CC1, value 0 */

    feed_and_parse(&inst, first, sizeof(first));

    /* Verify first callback */
    zassert_equal(g_last_ctrl, 1, "Expected controller 1");
    zassert_equal(g_last_val, 0, "Expected value 0");

    /* Now sweep CC1 from 1 → 127 using running status */
    for (uint8_t v = 1; v <= 127; v++) {
        uint8_t data[] = { 1, v };  /* controller=1, value=v */
        feed_and_parse(&inst, data, sizeof(data));

        /* Verify callback for each step */
        zassert_equal(g_last_ctrl, 1, "Expected controller 1");
        zassert_equal(g_last_val, v, "Expected value %d", v);
    }
}

ZTEST(midi1_parser, test_brutal_interleave_realtime_and_sysex)
{
    struct midi1_serial_inst inst;
    midi_test_inst_init(&inst);

    /* We will track the last CC and last note */
    g_last_ctrl = 0;
    g_last_val  = 0;
    g_last_note = 0;
    g_last_vel  = 0;
    g_last_rt   = 0;

    /* Brutal mixed stream:
     *
     * 1. CC1 = 0 (sets running status)
     * 2. CC1 = 1 (running status)
     * 3. Realtime F8
     * 4. CC1 = 2 (running status)
     * 5. SysEx start F0
     * 6. SysEx data bytes
     * 7. Realtime inside SysEx (legal)
     * 8. More SysEx data
     * 9. SysEx end F7
     * 10. CC1 = 3 (running status should resume)
     * 11. NOTE ON burst to verify parser recovery
     */

    uint8_t seq[] = {
        /* CC1 = 0 */
        0xB0, 1, 0,

        /* CC1 = 1 (running status) */
        1, 1,

        /* Realtime interleave */
        0xF8,

        /* CC1 = 2 (running status) */
        1, 2,

        /* SysEx start */
        0xF0,
        0x7D, 0x10, 0x20,  /* manufacturer + payload */

        /* Realtime inside SysEx */
        0xFA,

        /* More SysEx data */
        0x33, 0x44,

        /* SysEx end */
        0xF7,

	/* Running status does NOT resume — must send new status byte */
	0xB0, 1, 3, 

        /* NOTE ON burst to test recovery */
        0x90, 60, 100,
        62, 110,  /* running status NOTE ON */
    };

    feed_and_parse(&inst, seq, sizeof(seq));

    /* Validate last CC */
    zassert_equal(g_last_ctrl, 1, "Expected controller 1");
    zassert_equal(g_last_val, 3, "Expected CC value 3 after SysEx");

    /* Validate realtime inside SysEx */
    zassert_equal(g_last_rt, 0xFA, "Expected realtime FA inside SysEx");

    /* Validate NOTE ON recovery */
    zassert_equal(g_last_note, 62, "Expected last note 62");
    zassert_equal(g_last_vel, 110, "Expected last velocity 110");
}

/* Test suite entry point */
ZTEST_SUITE(midi1_parser, NULL, NULL, NULL, NULL, NULL);


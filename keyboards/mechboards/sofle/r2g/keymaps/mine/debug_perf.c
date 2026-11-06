// SPDX-License-Identifier: GPL-2.0-or-later
// On-keyboard performance debug. See debug_perf.h for what each stage covers.

#include QMK_KEYBOARD_H
#include <string.h>
#include "debug_perf.h"

static bool enabled;

// Current 1 s window
static uint32_t pass_start, stage_start, window_start, last_frame;
static uint8_t  stage;
static uint32_t stage_max[PERF_STAGES];
static uint32_t worst_pass, worst_gap, loops;
static uint8_t  worst_stage, frames;
static uint32_t stage_this_pass[PERF_STAGES];

static perf_stats_t last;
static uint32_t     window_no;

static inline uint32_t now_us(void) {
    return (uint32_t)chVTGetSystemTimeX(); // RP2040 TIMERAWL, 1 MHz
}

static void reset_window(uint32_t t) {
    memset(stage_max, 0, sizeof(stage_max));
    worst_pass = worst_gap = loops = 0;
    worst_stage                    = 0xFF;
    frames                         = 0;
    window_start                   = t;
}

void debug_perf_set_enabled(bool on) {
    if (on == enabled) return;
    enabled = on;
    if (on) {
        uint32_t t = now_us();
        reset_window(t);
        pass_start = stage_start = last_frame = t;
        stage                                 = PERF_SCAN;
        memset(stage_this_pass, 0, sizeof(stage_this_pass));
        memset(&last, 0, sizeof(last));
        last.hitch_stage = 0xFF;
    }
}

bool debug_perf_enabled(void) {
    return enabled;
}

const perf_stats_t *debug_perf_local(void) {
    return &last;
}

uint32_t debug_perf_window(void) {
    return window_no;
}

void debug_perf_mark(uint8_t next) {
    if (!enabled || next >= PERF_STAGES) return;
    uint32_t t = now_us();
    stage_this_pass[stage] += t - stage_start;
    stage       = next;
    stage_start = t;
}

// Lighting frame finished (QMK calls this once per frame, after the last LED batch).
bool rgb_matrix_indicators_user(void) {
    if (enabled) {
        uint32_t t   = now_us();
        uint32_t gap = t - last_frame;
        if (gap > worst_gap) worst_gap = gap;
        last_frame = t;
        if (frames < 255) frames++;
    }
    return true;
}

// Key scan + split sync just finished (each half runs one of these after the transport).
void matrix_scan_user(void) {
    debug_perf_mark(PERF_LIGHTS);
}
void matrix_slave_scan_user(void) {
    debug_perf_mark(PERF_LIGHTS);
}

static uint16_t clamp16(uint32_t v) {
    return v > 0xFFFF ? 0xFFFF : (uint16_t)v;
}

void debug_perf_loop(void) {
    if (!enabled) return;
    uint32_t t = now_us();

    // Close the pass: credit the time since the last mark to the stage it was in
    // ("other" after the display mark, or "lights" when the display task didn't run).
    stage_this_pass[stage] += t - stage_start;
    uint32_t pass = t - pass_start;

    uint8_t heaviest = 0;
    for (uint8_t s = 0; s < PERF_STAGES; s++) {
        if (stage_this_pass[s] > stage_max[s]) stage_max[s] = stage_this_pass[s];
        if (stage_this_pass[s] > stage_this_pass[heaviest]) heaviest = s;
    }
    if (pass > worst_pass) {
        worst_pass  = pass;
        worst_stage = heaviest;
    }
    loops++;
    memset(stage_this_pass, 0, sizeof(stage_this_pass));

    if (t - window_start >= 1000000) {
        last.fps             = frames;
        last.worst_gap_100us = clamp16(worst_gap / 100);
        last.loops_per_10    = clamp16(loops / 10);
        for (uint8_t s = 0; s < PERF_STAGES; s++) last.stage_max_10us[s] = clamp16(stage_max[s] / 10);
        last.hitch_100us = clamp16(worst_pass / 100);
        last.hitch_stage = worst_pass >= PERF_HITCH_US ? worst_stage : 0xFF;
        window_no++;
        reset_window(t);
    }

    // The next pass starts now; everything until the scan mark is "scan".
    t           = now_us(); // don't charge this bookkeeping to the next pass
    pass_start  = t;
    stage_start = t;
    stage       = PERF_SCAN;
}

// SPDX-License-Identifier: GPL-2.0-or-later
//
// On-keyboard performance debug: per-half lighting FPS, worst gap between
// lighting frames, main-loop rate, and the slowest pass through each stage of
// the main loop. Shown on the right screen's Debug page (settings_menu.c).
//
// Off unless the Debug page is open: then every hook is a flag check.
// Timing uses the RP2040's free-running 1 MHz counter (ChibiOS system time).
//
// A main-loop pass (quantum/main.c) is split into stages at existing hooks:
//   scan    start of pass -> matrix_scan_user / matrix_slave_scan_user
//           (key matrix scan + split cable round trip; it runs right after the transport)
//   lights  -> display_task_user start (rest of keyboard_task: rgb_matrix_task, encoders...)
//   screen  display_task_user start -> end (pet, album art, menu; includes SPI blits)
//   other   -> end of pass in housekeeping_task_user (USB/raw HID, QP animations,
//           deferred tasks, housekeeping, and this module's own stats sync)
// display_task_user runs as a deferred task 10x a second, so passes without it
// count everything after the scan as "lights"; that's where rgb_matrix_task is.

#pragma once

#include <stdbool.h>
#include <stdint.h>

enum { PERF_SCAN, PERF_LIGHTS, PERF_SCREEN, PERF_OTHER, PERF_STAGES };

// One second of measurements from one half. Packed: it rides in MENU_SYNC.
typedef struct __attribute__((packed)) {
    uint8_t  fps;                     // lighting frames finished
    uint8_t  hitch_stage;             // stage of the worst pass (PERF_*), or 0xFF if none over 5 ms
    uint16_t worst_gap_100us;         // longest gap between lighting frames, in 0.1 ms
    uint16_t loops_per_10;            // main-loop passes / 10
    uint16_t stage_max_10us[PERF_STAGES]; // slowest pass through each stage, in 10 us
    uint16_t hitch_100us;             // worst whole pass, in 0.1 ms
} perf_stats_t;

#define PERF_HITCH_US 5000 // passes slower than this are reported as a hitch

void               debug_perf_set_enabled(bool on);
bool               debug_perf_enabled(void);
void               debug_perf_loop(void);         // call first thing in housekeeping_task_user
void               debug_perf_mark(uint8_t stage); // a stage ends here (the next one begins)
const perf_stats_t *debug_perf_local(void);        // this half's last full window
uint32_t           debug_perf_window(void);        // bumps every time a new window is ready

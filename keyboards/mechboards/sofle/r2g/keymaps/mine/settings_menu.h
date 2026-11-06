// SPDX-License-Identifier: GPL-2.0-or-later
//
// Settings menu on the right LCD. The stock Mechboards RGB/LCD screen stays as it is;
// this adds an "Auto: ON/OFF" (autocorrect) row and a highlight that a Menu key
// (QK_KB_0, "Menu" in Vial's User tab) can move between rows:
//   Up/Down or K/J   move the highlight
//   Left/Right or H/L  adjust the highlighted setting
//   Enter / Space    toggle Auto, or step to the next RGB effect
//   Esc or Menu      close (any other key closes and types normally; 15 s idle closes)
//
// Key handling and setting changes run on the master (USB) half. The right half only
// draws, so its {open, cursor, autocorrect} state is sent across over split RPC MENU_SYNC.
// Needs MENU_SYNC in SPLIT_TRANSACTION_IDS_USER (config.h).

#pragma once

#include QMK_KEYBOARD_H

#define MENU_KEY QK_KB_0

void settings_menu_init(void);                                        // both halves, once at boot
bool settings_menu_process_record(uint16_t keycode, keyrecord_t *record); // false = key consumed
void settings_menu_master_task(void);                                 // master, from the display task
// Right half, from the display task. Returns true if the stock Mechboards screen
// should keep drawing, false while the Debug page owns the whole screen.
bool settings_menu_draw_right(void);

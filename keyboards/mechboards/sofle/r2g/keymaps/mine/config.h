/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#define VIAL_KEYBOARD_UID {0x95, 0xEB, 0xF9, 0x9C, 0x56, 0x05, 0x46, 0x82}

#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {0, 1}

#define SPLIT_TRANSACTION_IDS_USER KEYGLOW_SYNC, MENU_SYNC // keyglow + settings_menu.c

// Per-key deferred debounce (DEBOUNCE_TYPE in rules.mk): a key must stay
// down before it registers, so brief finger brushes are dropped. The timer
// ticks in whole ms, so 8 means a real wait of 7-8 ms (about 7.5 on average).
#define DEBOUNCE 8

// Lighting effect after a full reset, and after each new flash (keymap.c display_init_user).
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_BREATHING

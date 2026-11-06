// Copyright 2025 Dasky (@daskygit)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

// Copyright 2025 Dasky (@daskygit)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

// clang-format off

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT(
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                             KC_6,    KC_7,    KC_8,    KC_9,   KC_0,    KC_GRV,
    KC_ESC,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,                             KC_Y,    KC_U,    KC_I,    KC_O,   KC_P,    KC_BSPC,
    KC_TAB,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,                             KC_H,    KC_J,    KC_K,    KC_L,   KC_SCLN, KC_QUOT,
    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_LBRC,        KC_RBRC, KC_N,    KC_M,    KC_COMM, KC_DOT, KC_SLSH, KC_RSFT,
                      KC_LGUI, KC_LALT, KC_LCTL, MO(1),   KC_ENT,         KC_SPC,  MO(2),   KC_RCTL, KC_RALT, KC_RGUI,
                    //Left Encoder Press                                           Right Encoder Press
                            KC_MUTE,                                                         KC_MPLY
  ),
  [1] = LAYOUT(
    _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                            KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                             KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_F12,
    _______, KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC,                          KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_PIPE,
    _______, KC_EQL,  KC_MINS, KC_PLUS, KC_LCBR, KC_RCBR, KC_MUTE,        KC_MPLY, KC_LBRC, KC_RBRC, KC_SCLN, KC_COLN, KC_BSLS, _______,
                      _______, _______, _______, _______, _______,        _______, MO(3),   _______, _______, _______,
                    //Left Encoder Press                                           Right Encoder Press
                            KC_MUTE,                                                         KC_MPLY
  ),
  [2] = LAYOUT(
    _______, _______, _______, _______, _______, _______,                          _______, _______, _______, _______, _______ ,_______,
    _______, KC_INS,  KC_PSCR, KC_APP,  XXXXXXX, XXXXXXX,                          KC_PGUP, XXXXXXX, KC_UP,   XXXXXXX, XXXXXXX, KC_BSPC,
    _______, KC_LALT, KC_LCTL, KC_LSFT, XXXXXXX, KC_CAPS,                          KC_PGDN, KC_LEFT, KC_DOWN, KC_RGHT, KC_DEL,  XXXXXXX,
    _______, C(KC_Z), C(KC_X), C(KC_C), C(KC_V), XXXXXXX, _______,        _______, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, _______,
                      _______, _______, _______, MO(3),   _______,        _______, _______, _______, _______, _______,
                    //Left Encoder Press                                           Right Encoder Press
                            KC_MUTE,                                                         KC_MPLY
  ),
    [3] = LAYOUT(
    XXXXXXX, XXXXXXX, XXXXXXX ,XXXXXXX, XXXXXXX, XXXXXXX,                          XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
    QK_BOOT, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                          XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
    RM_TOGG, RM_HUEU, RM_SATU, RM_VALU, XXXXXXX, XXXXXXX,                          XXXXXXX, KC_VOLD, KC_MUTE, KC_VOLU, XXXXXXX, XXXXXXX,
    RM_NEXT, RM_HUED, RM_SATD, RM_VALD, XXXXXXX, XXXXXXX, XXXXXXX,        XXXXXXX, XXXXXXX, KC_MPRV, KC_MPLY, KC_MNXT, XXXXXXX, XXXXXXX,
                      _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______,
                    //Left Encoder Press                                           Right Encoder Press
                            KC_MUTE,                                                         KC_MPLY
  )
};

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
[0] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_MPRV, KC_MNXT)},
[1] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
[2] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
[3] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS)}
};
#endif
// clang-format on

// ---------------------------------------------------------------------------
// Pikachu keyboard pet on the left screen (Luna-style). The right screen keeps
// the stock Mechboards display. Each screen is a 135x240 colour LCD; the pet
// animates in the top 135x190 and the layer name uses the bottom 50 rows.
// Colours are HSV 0-255: hue 0=red, 85=green, 170=blue; sat 0 + val 255 = white.
// ---------------------------------------------------------------------------
#include "common/display_lcd.h"
#include "pika_sit.qgf.h"
#include "pika_walk.qgf.h"
#include "pika_run.qgf.h"
#include "pika_jump.qgf.h"
#include "pika_sneak.qgf.h"
#include "pika_zap.qgf.h"
#include "pika_sleep.qgf.h"
#include "pika_meta.h" // PIKA_JUMP_MS, PIKA_ZAP_MS (and PIKA_WAKE_MS if the style has a wake-up)
#include "settings_menu.h" // right-screen settings menu (Menu key, autocorrect toggle)
#include "debug_perf.h"    // per-stage timing for the menu's Debug page
#ifdef PIKA_WAKE_MS
#    include "pika_wake.qgf.h"
#endif

#define PIKA_WALK_WPM 10  // above this: walk
#define PIKA_RUN_WPM 50   // above this: run
#define PIKA_SLEEP_MS 60000
#define PET_HEIGHT 190
#define LCD_X 52 // the LCD panel's offset in controller RAM; lcd itself has no offset set
#define LCD_Y 40

enum pika_state { PIKA_SIT, PIKA_WALK, PIKA_RUN, PIKA_JUMP, PIKA_SNEAK, PIKA_ZAP, PIKA_SLEEP, PIKA_WAKE, PIKA_COUNT };

static painter_image_handle_t pika_img[PIKA_COUNT];
static deferred_token         pika_anim = INVALID_DEFERRED_TOKEN;
static uint8_t                pika_oneshot = PIKA_COUNT; // PIKA_JUMP/PIKA_ZAP/PIKA_WAKE while playing, else PIKA_COUNT
static uint32_t               pika_oneshot_start;

static const char   *layer_names[] = {"Base", "Lower", "Raise", "Adjust"};
static const uint8_t layer_hues[]  = {170, 85, 0, 213};

static bool ac_enable_after_reset;

bool display_init_user(void) {
    if (ac_enable_after_reset) {
        autocorrect_enable(); // see via_init_kb() below
    }
    pika_img[PIKA_SIT]   = qp_load_image_mem(gfx_pika_sit);
    pika_img[PIKA_WALK]  = qp_load_image_mem(gfx_pika_walk);
    pika_img[PIKA_RUN]   = qp_load_image_mem(gfx_pika_run);
    pika_img[PIKA_JUMP]  = qp_load_image_mem(gfx_pika_jump);
    pika_img[PIKA_SNEAK] = qp_load_image_mem(gfx_pika_sneak);
    pika_img[PIKA_ZAP]   = qp_load_image_mem(gfx_pika_zap);
    pika_img[PIKA_SLEEP] = qp_load_image_mem(gfx_pika_sleep);
#ifdef PIKA_WAKE_MS
    pika_img[PIKA_WAKE] = qp_load_image_mem(gfx_pika_wake);
#endif
    settings_menu_init();
    return true; // returning false would skip the rest of the display setup
}

static void pika_start_oneshot(uint8_t state) {
    if (pika_oneshot == state) {
        return; // already playing, don't restart mid-animation
    }
    pika_oneshot       = state;
    pika_oneshot_start = timer_read32();
}

static void pika_on_tap_keycode(uint16_t keycode) {
    switch (keycode) {
        case KC_SPC:
            pika_start_oneshot(PIKA_JUMP);
            break;
        case KC_ESC:
            pika_start_oneshot(PIKA_ZAP);
            break;
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!settings_menu_process_record(keycode, record)) {
        return false; // the menu used this key
    }
    if (IS_QK_TAP_DANCE(keycode)) {
        // Vial tap dances (e.g. Space-tap/Shift-hold) send their key without passing through here,
        // so look up the tap key and count it as a tap if the key was released within the tapping term.
        static uint16_t td_pressed_at;
        if (record->event.pressed) {
            td_pressed_at = record->event.time;
            return true;
        }
        vial_tap_dance_entry_t td;
        if (dynamic_keymap_get_tap_dance(QK_TAP_DANCE_GET_INDEX(keycode), &td) == 0 && TIMER_DIFF_16(record->event.time, td_pressed_at) < td.custom_tapping_term) {
            pika_on_tap_keycode(td.on_tap);
        }
        return true;
    }
    if (IS_QK_MOD_TAP(keycode) || IS_QK_LAYER_TAP(keycode)) {
        if (!record->event.pressed && record->tap.count) {
            pika_on_tap_keycode(keycode & 0xFF);
        }
        return true;
    }
    if (record->event.pressed) {
        pika_on_tap_keycode(keycode);
    }
    return true;
}

// Autocorrect uses QMK's built-in typo list (quantum/process_keycode/autocorrect_data_default.h).
// It can't see what a Vial tap dance types, so TD(2) (tap = Space) would wipe its memory instead
// of counting as a word break. Hand it the tap dance's tap key instead.
bool process_autocorrect_user(uint16_t *keycode, keyrecord_t *record, uint8_t *typo_buffer_size, uint8_t *mods) {
    if (IS_QK_TAP_DANCE(*keycode)) {
        vial_tap_dance_entry_t td;
        if (dynamic_keymap_get_tap_dance(QK_TAP_DANCE_GET_INDEX(*keycode), &td) == 0 && IS_BASIC_KEYCODE(td.on_tap)) {
            *keycode = td.on_tap;
        }
    }
    return process_autocorrect_default_handler(keycode, record, typo_buffer_size, mods);
}

// Each new firmware build makes Vial reset its settings (quantum/dynamic_keymap.c ->
// qmk_settings_reset), which zeroes keymap_config and so switches autocorrect off.
// via_init_kb() runs just before that reset, so note it here and switch autocorrect back
// on once boot finishes (display_init_user, called from r2g.c's keyboard_post_init_kb).
// An AC_TOGG off still persists until the next flash.
void via_init_kb(void) {
    ac_enable_after_reset = !via_eeprom_is_valid();
}

static uint8_t pika_pick_state(void) {
    if (pika_oneshot != PIKA_COUNT) {
        uint32_t length = pika_oneshot == PIKA_JUMP ? PIKA_JUMP_MS : PIKA_ZAP_MS;
#ifdef PIKA_WAKE_MS
        if (pika_oneshot == PIKA_WAKE) length = PIKA_WAKE_MS;
#endif
        if (timer_elapsed32(pika_oneshot_start) < length) {
            return pika_oneshot;
        }
        pika_oneshot = PIKA_COUNT;
    }
    if (get_mods() & MOD_MASK_CTRL) return PIKA_SNEAK;
    if (last_input_activity_elapsed() > PIKA_SLEEP_MS) return PIKA_SLEEP;
    uint8_t wpm = get_current_wpm();
    if (wpm > PIKA_RUN_WPM) return PIKA_RUN;
    if (wpm > PIKA_WALK_WPM) return PIKA_WALK;
    return PIKA_SIT;
}

// ---------------------------------------------------------------------------
// Album art (KeyGlow v3): while Apple Music plays, the cover plus title and
// artist replace the pet in the top 135x190. The pet comes back ART_GRACE_MS
// after the music stops or pauses, or if the Mac goes quiet (app quit, sleep).
// Everything is drawn straight to the panel, and only when something changes.
// ---------------------------------------------------------------------------
#include "keyglow.h"
#if KEYGLOW_ART_SIDE > 0
#    define ART_GRACE_MS 5000
#    define ART_HOST_TIMEOUT_MS 20000 // the Mac re-sends track info at least this often while playing
#    define ART_X ((135 - KEYGLOW_ART_SIDE) / 2)
#    define ART_Y 8
#    define ART_TITLE_Y (ART_Y + KEYGLOW_ART_SIDE + 8)
#    define ART_ARTIST_Y (ART_TITLE_Y + 22)

static bool art_wanted(void) {
    static uint32_t last_playing;
    static bool     ever_played;
    if (!keyglow_art_ready()) return false;
    if (keyglow_player_state() == KEYGLOW_PLAYER_PLAYING && keyglow_art_idle_ms() < ART_HOST_TIMEOUT_MS) {
        last_playing = timer_read32();
        ever_played  = true;
        return true;
    }
    return ever_played && timer_elapsed32(last_playing) < ART_GRACE_MS;
}

// The panel wants RGB565 big-endian; the buffer is little-endian (as surfaces use).
static void art_blit(void) {
    static uint8_t rows[QUANTUM_PAINTER_PIXDATA_BUFFER_SIZE];
    const uint16_t row_bytes      = KEYGLOW_ART_SIDE * 2;
    const uint16_t rows_per_batch = sizeof(rows) / row_bytes;
    qp_viewport(lcd, LCD_X + ART_X, LCD_Y + ART_Y, LCD_X + ART_X + KEYGLOW_ART_SIDE - 1, LCD_Y + ART_Y + KEYGLOW_ART_SIDE - 1);
    for (uint16_t y = 0; y < KEYGLOW_ART_SIDE; y += rows_per_batch) {
        uint16_t       n   = KEYGLOW_ART_SIDE - y < rows_per_batch ? KEYGLOW_ART_SIDE - y : rows_per_batch;
        const uint8_t *src = &keyglow_art_pixels[(uint32_t)y * row_bytes];
        for (uint32_t i = 0; i < (uint32_t)n * row_bytes; i += 2) {
            rows[i]     = src[i + 1];
            rows[i + 1] = src[i];
        }
        qp_pixdata(lcd, rows, (uint32_t)n * KEYGLOW_ART_SIDE);
    }
}

// Draws `text` centred and cut to fit 131 px, with "..." when it doesn't.
static void art_text(uint16_t y, const char *text, uint8_t val) {
    char    buf[KEYGLOW_TRACK_TEXT + 4];
    uint8_t len = strlen(text);
    memcpy(buf, text, len + 1);
    while (len > 0 && qp_textwidth(pixellari_18, buf) > 131) {
        len--;
        while (len > 0 && buf[len - 1] == ' ') len--;
        memcpy(&buf[len], "...", 4);
        if (qp_textwidth(pixellari_18, buf) <= 131) break;
    }
    qp_drawtext_recolor(lcd, LCD_X + (135 - qp_textwidth(pixellari_18, buf)) / 2, LCD_Y + y, pixellari_18, buf, 0, 0, val, 0, 0, 0);
}

static void art_draw(void) {
    qp_rect(lcd, LCD_X, LCD_Y, LCD_X + 134, LCD_Y + PET_HEIGHT - 1, 0, 0, 0, true);
    art_blit();
    art_text(ART_TITLE_Y, keyglow_track_title(), 255);
    art_text(ART_ARTIST_Y, keyglow_track_artist(), 150);
}
#endif

static void draw_status_strip(uint8_t layer) {
    // Only touches rows PET_HEIGHT..239 of the surface, so qp_surface_draw never paints over the pet.
    qp_rect(surface, 0, PET_HEIGHT, 134, 239, 0, 0, 0, true);
    const char *name = layer < 4 ? layer_names[layer] : "?";
    qp_drawtext_recolor(surface, (135 - qp_textwidth(pixellari_24, name)) / 2, PET_HEIGHT + 13, pixellari_24, name, layer < 4 ? layer_hues[layer] : 0, 255, 255, 0, 0, 0);
    qp_surface_draw(surface, lcd, LCD_X, LCD_Y, false);
}

bool display_task_user(void) {
    debug_perf_mark(PERF_SCREEN); // everything from here to the end of this function is "screen"
    if (is_keyboard_master()) {
        settings_menu_master_task(); // menu timeout/repeat + send its state to the right half
    }
    if (!is_keyboard_left()) {
        // Auto row + highlights over the stock screen, or the whole Debug page.
        bool stock = settings_menu_draw_right();
        debug_perf_mark(PERF_OTHER);
        return stock;
    }

    static uint8_t last_state = PIKA_COUNT;
#if KEYGLOW_ART_SIDE > 0
    static bool     showing_art;
    static uint16_t drawn_rev;
    if (art_wanted()) {
        if (!showing_art) {
            showing_art = true;
            qp_stop_animation(pika_anim);
            pika_anim = INVALID_DEFERRED_TOKEN;
            drawn_rev = keyglow_art_revision() - 1; // force a draw
        }
        if (keyglow_art_revision() != drawn_rev) {
            drawn_rev = keyglow_art_revision();
            art_draw();
        }
        goto status_strip;
    }
    if (showing_art) {
        showing_art = false;
        qp_rect(lcd, LCD_X, LCD_Y, LCD_X + 134, LCD_Y + PET_HEIGHT - 1, 0, 0, 0, true);
        last_state = PIKA_COUNT; // restart the pet below
    }
#endif
    uint8_t state = pika_pick_state();
#ifdef PIKA_WAKE_MS
    if (last_state == PIKA_SLEEP && state != PIKA_SLEEP && state != PIKA_WAKE) {
        pika_start_oneshot(PIKA_WAKE); // get up before doing anything else
        state = PIKA_WAKE;
    }
#endif
    if (state != last_state) {
        last_state = state;
        qp_stop_animation(pika_anim);
        pika_anim = qp_animate(lcd, LCD_X, LCD_Y, pika_img[state]);
    }

#if KEYGLOW_ART_SIDE > 0
status_strip:;
#endif
    static int8_t last_layer = -1;
    uint8_t       layer      = get_highest_layer(layer_state);
    if (layer != last_layer) {
        last_layer = layer;
        draw_status_strip(layer);
    }
    debug_perf_mark(PERF_OTHER);
    return false;
}

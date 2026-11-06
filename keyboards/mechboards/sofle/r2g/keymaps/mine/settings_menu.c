// SPDX-License-Identifier: GPL-2.0-or-later
// Settings menu on the right LCD. See settings_menu.h for the controls.

#include "settings_menu.h"
#include <stdio.h>
#include <string.h>
#include "common/display_lcd.h"
#include "split_util.h"
#include "transactions.h"
#include "debug_perf.h"

#define MENU_TIMEOUT_MS 15000
#define REPEAT_DELAY_MS 400 // holding Left/Right repeats after this, once per display frame (100 ms)
#define SYNC_HEARTBEAT_MS 1000

// ROW_DEBUG is drawn as a "Debug >" line in the free space under the Bri bar.
enum { ROW_EFFECT, ROW_HUE, ROW_SAT, ROW_VAL, ROW_AUTO, ROW_BRI, ROW_DEBUG, ROW_COUNT };
enum { ACT_NONE, ACT_UP, ACT_DOWN, ACT_LEFT, ACT_RIGHT, ACT_SELECT, ACT_CLOSE };
enum { PAGE_MENU, PAGE_DEBUG };

// What the right half needs to draw. autocorrect: 0 off, 1 on, 2 not heard from master yet.
// `master` is the USB half's perf stats (only filled while the Debug page is open).
typedef struct __attribute__((packed)) {
    uint8_t      open;
    uint8_t      cursor;
    uint8_t      autocorrect;
    uint8_t      page;
    uint32_t     master_window; // bumps when `master` holds a new 1 s window
    perf_stats_t master;
} menu_sync_t;
_Static_assert(sizeof(menu_sync_t) <= 32, "MENU_SYNC must fit RPC_M2S_BUFFER_SIZE");

static menu_sync_t view = {.cursor = ROW_AUTO, .autocorrect = 2}; // drawn by the right half (RPC handler writes it)

// ---------------------------------------------------------------------------
// Master half: key handling and setting changes
// ---------------------------------------------------------------------------
static bool     menu_open;
static uint8_t  cursor = ROW_AUTO;
static uint8_t  page   = PAGE_MENU;
static uint32_t last_input;
static uint8_t  swallowed[MATRIX_ROWS]; // presses we ate, so their releases get eaten too
_Static_assert(MATRIX_COLS <= 8, "swallowed[] packs one row per byte");

static uint8_t  held_act; // Left/Right being held for repeat
static keypos_t held_pos;
static uint32_t held_since;

static uint8_t  td_act; // tap dance key waiting for its release to tell tap from hold
static keypos_t td_pos;
static uint16_t td_time;
static uint16_t td_term;

static void menu_set_open(bool open) {
    menu_open  = open;
    cursor     = ROW_AUTO;
    page       = PAGE_MENU;
    held_act   = ACT_NONE;
    td_act     = ACT_NONE;
    last_input = timer_read32();
}

static uint8_t action_for(uint16_t kc) {
    switch (kc) {
        case KC_UP:
        case KC_K:
            return ACT_UP;
        case KC_DOWN:
        case KC_J:
            return ACT_DOWN;
        case KC_LEFT:
        case KC_H:
            return ACT_LEFT;
        case KC_RIGHT:
        case KC_L:
            return ACT_RIGHT;
        case KC_ENTER:
        case KC_KP_ENTER:
        case KC_SPACE:
            return ACT_SELECT;
        case KC_ESCAPE:
            return ACT_CLOSE;
        default:
            return ACT_NONE;
    }
}

static void adjust(int8_t dir) {
    switch (cursor) {
        case ROW_EFFECT:
            dir > 0 ? rgb_matrix_step() : rgb_matrix_step_reverse();
            break;
        case ROW_HUE:
            dir > 0 ? rgb_matrix_increase_hue() : rgb_matrix_decrease_hue();
            break;
        case ROW_SAT:
            dir > 0 ? rgb_matrix_increase_sat() : rgb_matrix_decrease_sat();
            break;
        case ROW_VAL:
            dir > 0 ? rgb_matrix_increase_val() : rgb_matrix_decrease_val();
            break;
        case ROW_AUTO:
            autocorrect_toggle();
            break;
        case ROW_BRI:
            dir > 0 ? backlight_increase() : backlight_decrease();
            break;
    }
}

static void do_action(uint8_t act) {
    last_input = timer_read32();
    if (page == PAGE_DEBUG) {
        if (act == ACT_CLOSE) page = PAGE_MENU; // Esc: back to the menu, Debug row still selected
        return;
    }
    switch (act) {
        case ACT_UP:
            cursor = (cursor + ROW_COUNT - 1) % ROW_COUNT;
            break;
        case ACT_DOWN:
            cursor = (cursor + 1) % ROW_COUNT;
            break;
        case ACT_LEFT:
            adjust(-1);
            break;
        case ACT_RIGHT:
            adjust(+1);
            break;
        case ACT_SELECT:
            if (cursor == ROW_AUTO) autocorrect_toggle();
            if (cursor == ROW_EFFECT) rgb_matrix_step();
            if (cursor == ROW_DEBUG) page = PAGE_DEBUG;
            break;
        case ACT_CLOSE:
            menu_set_open(false);
            break;
    }
}

// Layer and modifier keys don't close the menu, so e.g. holding a layer key for the arrows works.
static bool is_layer_or_mod(uint16_t kc) {
    return IS_MODIFIER_KEYCODE(kc) || IS_QK_MOMENTARY(kc) || IS_QK_LAYER_TAP(kc) || IS_QK_LAYER_MOD(kc) || IS_QK_ONE_SHOT_MOD(kc) || IS_QK_ONE_SHOT_LAYER(kc) || IS_QK_TO(kc) || IS_QK_TOGGLE_LAYER(kc) || IS_QK_LAYER_TAP_TOGGLE(kc) || IS_QK_MOD_TAP(kc);
}

bool settings_menu_process_record(uint16_t keycode, keyrecord_t *record) {
    if (!IS_KEYEVENT(record->event)) return true;
    keypos_t pos       = record->event.key;
    bool     in_matrix = pos.row < MATRIX_ROWS && pos.col < MATRIX_COLS; // Vial tap dance output uses row 240

    if (!record->event.pressed) {
        if (!in_matrix || !(swallowed[pos.row] & (1 << pos.col))) {
            return true; // not ours, e.g. a layer key that was already held when the menu opened
        }
        swallowed[pos.row] &= ~(1 << pos.col);
        if (held_act && held_pos.row == pos.row && held_pos.col == pos.col) {
            held_act = ACT_NONE;
        }
        if (td_act && td_pos.row == pos.row && td_pos.col == pos.col) {
            uint8_t act = td_act;
            td_act      = ACT_NONE;
            if (menu_open && TIMER_DIFF_16(record->event.time, td_time) < td_term) {
                do_action(act); // a quick tap; a hold does nothing while the menu is open
            }
        }
        return false;
    }

    if (keycode == MENU_KEY) {
        menu_set_open(!menu_open);
        if (in_matrix) swallowed[pos.row] |= 1 << pos.col;
        return false;
    }
    if (!menu_open) return true;

    // On the Debug page the keyboard types normally, so you can watch the numbers
    // while typing; only Esc (a quick tap of the Esc tap dance) goes back to the menu.
    if (page == PAGE_DEBUG) {
        uint16_t tap = keycode;
        if (IS_QK_TAP_DANCE(keycode)) {
            vial_tap_dance_entry_t td;
            tap = dynamic_keymap_get_tap_dance(QK_TAP_DANCE_GET_INDEX(keycode), &td) == 0 ? td.on_tap : KC_NO;
            if (tap == KC_ESCAPE && in_matrix) {
                td_act  = ACT_CLOSE; // decided at release, so holding Esc for Ctrl still works
                td_pos  = pos;
                td_time = record->event.time;
                td_term = td.custom_tapping_term;
                swallowed[pos.row] |= 1 << pos.col;
                return false;
            }
            return true;
        }
        if (tap == KC_ESCAPE) {
            if (in_matrix) swallowed[pos.row] |= 1 << pos.col;
            do_action(ACT_CLOSE);
            return false;
        }
        return true;
    }

    // Vial tap dances (Esc/Ctrl, Enter/Adjust, Space/Shift): act on the tap key, decided at release.
    if (IS_QK_TAP_DANCE(keycode)) {
        vial_tap_dance_entry_t td;
        uint8_t                act = ACT_NONE;
        if (dynamic_keymap_get_tap_dance(QK_TAP_DANCE_GET_INDEX(keycode), &td) == 0) {
            act = action_for(td.on_tap);
        }
        if (act != ACT_NONE && in_matrix) {
            td_act  = act;
            td_pos  = pos;
            td_time = record->event.time;
            td_term = td.custom_tapping_term;
            swallowed[pos.row] |= 1 << pos.col;
            return false;
        }
        menu_set_open(false);
        return true;
    }

    uint8_t act = action_for(keycode);
    if (act == ACT_NONE) {
        if (!is_layer_or_mod(keycode)) menu_set_open(false); // any other key closes the menu and types
        return true;
    }
    if (in_matrix) swallowed[pos.row] |= 1 << pos.col;
    do_action(act);
    // Hold-to-repeat only makes sense for the sliders, not for flipping Auto or stepping effects.
    if ((act == ACT_LEFT || act == ACT_RIGHT) && cursor != ROW_AUTO && cursor != ROW_EFFECT) {
        held_act   = act;
        held_pos   = pos;
        held_since = timer_read32();
    }
    return false;
}

void settings_menu_master_task(void) {
    if (menu_open) {
        if (held_act && timer_elapsed32(held_since) > REPEAT_DELAY_MS) {
            do_action(held_act);
        }
        // The debug page is for watching, so it doesn't time out.
        if (page != PAGE_DEBUG && timer_elapsed32(last_input) > MENU_TIMEOUT_MS) {
            menu_set_open(false);
        }
    }

    bool debugging = menu_open && page == PAGE_DEBUG;
    debug_perf_set_enabled(debugging); // the slave follows `page` in menu_sync_handler

    menu_sync_t s = {
        .open        = menu_open,
        .cursor      = cursor,
        .autocorrect = autocorrect_is_enabled(),
        .page        = debugging ? PAGE_DEBUG : PAGE_MENU,
    };
    if (debugging) {
        s.master_window = debug_perf_window();
        s.master        = *debug_perf_local();
    }
    view = s; // in case this half is also the one drawing (USB in the right half)

    static menu_sync_t last_sent = {.open = 0xFF};
    static uint32_t    last_send;
    if (memcmp(&s, &last_sent, sizeof(s)) != 0 || timer_elapsed32(last_send) > SYNC_HEARTBEAT_MS) {
        if (is_transport_connected() && transaction_rpc_send(MENU_SYNC, sizeof(s), &s)) {
            last_sent = s;
            last_send = timer_read32();
        }
    }
}

// ---------------------------------------------------------------------------
// Right half: draw the Auto row and highlights over the stock screen
// ---------------------------------------------------------------------------
static void menu_sync_handler(uint8_t in_len, const void *in, uint8_t out_len, void *out) {
    if (in_len == sizeof(menu_sync_t)) {
        memcpy(&view, in, sizeof(view));
        debug_perf_set_enabled(view.page == PAGE_DEBUG); // measure this half too while the page is up
    }
}

void settings_menu_init(void) {
    transaction_register_rpc(MENU_SYNC, menu_sync_handler);
}

// Stock layout (common/display_lcd.c): "RGB" title y=10 (24px font, centred), effect name y=40-98,
// "Hue:" y=110, "Sat:" y=130, "Val:" y=150, "LCD" title y=190, "Bri:" y=220; bars start at x=45.
// y=170 is free, so the Auto row goes there.
#define AUTO_Y 170
#define LABEL_MAX_X 43 // keep label highlights clear of the bars at x=45

static void draw_label(painter_font_handle_t font, uint16_t x, uint16_t y, uint16_t max_y, const char *text, bool highlighted) {
    uint16_t w  = qp_textwidth(font, text);
    uint16_t x0 = x >= 3 ? x - 3 : 0;
    uint16_t x1 = x + w + 2;
    if (x == 0 && x1 > LABEL_MAX_X) x1 = LABEL_MAX_X;
    uint8_t fg = highlighted ? 0 : 255; // black on white when highlighted, white on black otherwise
    uint8_t bg = highlighted ? 255 : 0;
    qp_rect(surface, x0, y - 1, x1, max_y, 0, 0, bg, true);
    qp_drawtext_recolor(surface, x, y, font, text, 0, 0, fg, 0, 0, bg);
    if (y + font->line_height - 1 > max_y) {
        // The glyph background runs below max_y; trim it so it matches the box (stock clears y=30+ anyway).
        qp_rect(surface, x0, max_y + 1, x1, y + font->line_height - 1, 0, 0, 0, true);
    }
}

// Draws one row of the overlay. Rows are drawn individually and only when they
// change: the surface keeps a single dirty rectangle, so touching the "RGB" title
// (y=10) and "Bri:" (y=220) in the same frame would push the whole 135x230 screen
// over SPI (~40 ms with the main loop blocked, freezing this half's lights).
static void draw_row(uint8_t row, const menu_sync_t *v) {
    bool hl = v->open && v->cursor == row;
    switch (row) {
        case ROW_EFFECT: {
            uint16_t title_x = 135 / 2 - qp_textwidth(pixellari_24, "RGB") / 2; // same centring as drawtext_centered
            draw_label(pixellari_24, title_x, 10, 29, "RGB", hl);
            break;
        }
        case ROW_HUE:
            draw_label(pixellari_18, 0, 110, 127, "Hue:", hl);
            break;
        case ROW_SAT:
            draw_label(pixellari_18, 0, 130, 147, "Sat:", hl);
            break;
        case ROW_VAL:
            draw_label(pixellari_18, 0, 150, 167, "Val:", hl);
            break;
        case ROW_BRI:
            draw_label(pixellari_18, 0, 220, 237, "Bri:", hl);
            break;
        case ROW_DEBUG: {
            // Shares the stock "LCD" title row (y=190): the screen has no free row left.
            // Highlighted, it reads "Debug >" so it's clear Enter opens a page.
            const char *text = hl ? "Debug >" : "LCD";
            qp_rect(surface, 0, 189, 134, 189 + pixellari_24->line_height, 0, 0, 0, true);
            uint16_t x = 135 / 2 - qp_textwidth(pixellari_24, text) / 2;
            draw_label(pixellari_24, x, 190, 189 + pixellari_24->line_height - 1, text, hl);
            break;
        }
        case ROW_AUTO: {
            qp_rect(surface, LABEL_MAX_X + 1, AUTO_Y - 1, 134, AUTO_Y + pixellari_18->line_height - 1, 0, 0, 0, true);
            draw_label(pixellari_18, 0, AUTO_Y, AUTO_Y + 17, "Auto:", hl);
            const char *state = v->autocorrect == 1 ? "ON" : v->autocorrect == 0 ? "OFF" : "--";
            uint8_t     h = 0, s = 0, val = 110; // grey for OFF / unknown
            if (v->autocorrect == 1) {
                h   = rgb_matrix_get_hue(); // the stock screen's accent colour
                s   = rgb_matrix_get_sat();
                val = 255;
            }
            qp_drawtext_recolor(surface, 135 - qp_textwidth(pixellari_18, state), AUTO_Y, pixellari_18, state, h, s, val, 0, 0, 0);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Debug page: per-half perf numbers (debug_perf.h). Drawn line by line, and a
// line only when its text changes, so the page itself never pushes a big
// dirty rectangle (that would be the very stall it's trying to show).
// ---------------------------------------------------------------------------
#define DBG_LINES 10
#define DBG_LINE_H 21
#define DBG_TOP 4

static const char *const stage_names[PERF_STAGES] = {"scan", "lights", "screen", "other"};

static void fmt_ms(char *out, size_t n, uint16_t tenths_or_hundredths, bool hundredths_of_ms) {
    // stage_max is in 10 us (0.01 ms); gaps and hitches are in 100 us (0.1 ms)
    if (hundredths_of_ms) {
        snprintf(out, n, "%u.%02u", tenths_or_hundredths / 100, tenths_or_hundredths % 100);
    } else {
        snprintf(out, n, "%u.%u", tenths_or_hundredths / 10, tenths_or_hundredths % 10);
    }
}

static void fmt_k(char *out, size_t n, uint16_t per10) {
    uint32_t v = (uint32_t)per10 * 10;
    if (v >= 1000) {
        snprintf(out, n, "%lu.%luk", (unsigned long)(v / 1000), (unsigned long)(v % 1000 / 100));
    } else {
        snprintf(out, n, "%lu", (unsigned long)v);
    }
}

// Builds the text of each line: label, left value, right value.
static void debug_line(uint8_t i, const perf_stats_t *l, const perf_stats_t *r, char *label, char *lv, char *rv) {
    label[0] = lv[0] = rv[0] = 0;
    switch (i) {
        case 0:
            strcpy(label, "DEBUG");
            strcpy(lv, "L");
            strcpy(rv, "R");
            break;
        case 1:
            strcpy(label, "fps");
            snprintf(lv, 8, "%u", l->fps);
            snprintf(rv, 8, "%u", r->fps);
            break;
        case 2:
            strcpy(label, "gap ms");
            fmt_ms(lv, 8, l->worst_gap_100us, false);
            fmt_ms(rv, 8, r->worst_gap_100us, false);
            break;
        case 3:
            strcpy(label, "loops");
            fmt_k(lv, 8, l->loops_per_10);
            fmt_k(rv, 8, r->loops_per_10);
            break;
        case 4:
            strcpy(label, "max ms");
            break;
        case 5:
        case 6:
        case 7:
        case 8: {
            uint8_t s = i - 5;
            strcpy(label, stage_names[s]);
            fmt_ms(lv, 8, l->stage_max_10us[s], true);
            fmt_ms(rv, 8, r->stage_max_10us[s], true);
            break;
        }
        case 9: {
            // The worst hitch of the two halves, or "ok".
            const perf_stats_t *h    = NULL;
            char                side = 0;
            if (l->hitch_stage < PERF_STAGES) h = l, side = 'L';
            if (r->hitch_stage < PERF_STAGES && (!h || r->hitch_100us > h->hitch_100us)) h = r, side = 'R';
            if (h) {
                char ms[8];
                fmt_ms(ms, sizeof(ms), h->hitch_100us, false);
                snprintf(label, 16, "hitch %c", side);
                snprintf(lv, 8, "%s", stage_names[h->hitch_stage]);
                snprintf(rv, 8, "%s", ms);
            } else {
                strcpy(label, "hitch");
                strcpy(lv, "");
                strcpy(rv, "none");
            }
            break;
        }
    }
}

static void draw_debug_page(const perf_stats_t *l, const perf_stats_t *r, bool full) {
    static char shown[DBG_LINES][32];
    if (full) {
        qp_rect(surface, 0, 0, 134, 239, 0, 0, 0, true);
        memset(shown, 0, sizeof(shown));
    }
    for (uint8_t i = 0; i < DBG_LINES; i++) {
        char label[16], lv[8], rv[8], key[32];
        debug_line(i, l, r, label, lv, rv);
        snprintf(key, sizeof(key), "%s|%s|%s", label, lv, rv);
        if (strcmp(key, shown[i]) == 0) continue;
        strcpy(shown[i], key);

        uint16_t y      = DBG_TOP + i * DBG_LINE_H;
        bool     header = i == 0 || i == 4;
        uint8_t  val    = header ? 150 : 255;
        bool     bad    = i == 9 && strcmp(rv, "none") != 0;
        qp_rect(surface, 0, y, 134, y + DBG_LINE_H - 1, 0, 0, 0, true);
        qp_drawtext_recolor(surface, 2, y, pixellari_18, label, 0, 0, val, 0, 0, 0);
        // Left column right-aligned at x=92, right column right-aligned at x=133.
        qp_drawtext_recolor(surface, 92 - qp_textwidth(pixellari_18, lv), y, pixellari_18, lv, 0, bad ? 255 : 0, val, 0, 0, 0);
        qp_drawtext_recolor(surface, 133 - qp_textwidth(pixellari_18, rv), y, pixellari_18, rv, 0, bad ? 255 : 0, val, 0, 0, 0);
        // Push this line now so each SPI transfer stays one 135x21 strip (~1.5 ms).
        qp_surface_draw(surface, lcd, 52, 40, false);
    }
}

void display_ui_init(void); // common/display_lcd.c: repaints the stock right screen

bool settings_menu_draw_right(void) {
    menu_sync_t v;
    memcpy(&v, &view, sizeof(v));

    static bool on_debug;
    if (v.page == PAGE_DEBUG) {
        static uint32_t drawn_local = UINT32_MAX, drawn_master = UINT32_MAX;
        bool            full = !on_debug;
        on_debug             = true;
        // Redraw when either half has a new 1 s window (or on entry).
        if (full || debug_perf_window() != drawn_local || v.master_window != drawn_master) {
            drawn_local  = debug_perf_window();
            drawn_master = v.master_window;
            // This (right) half drew the screen, so the master's numbers are "L".
            draw_debug_page(&v.master, debug_perf_local(), full);
        }
        return false; // the stock screen stays off while the page is up
    }
    static bool repaint_overlay;
    if (on_debug) {
        // Back to the menu: rebuild the stock screen (a one-off full push, ~16 ms),
        // then redraw every overlay row on top of it below.
        on_debug        = false;
        repaint_overlay = true;
        display_ui_init();
    }

    // When the RGB hue/sat changes, the stock code clears and repaints the whole screen
    // *after* this runs (display_ui_init), wiping the overlay; that full repaint is the
    // stock code's own cost. Redraw every row on that frame and the next one. Otherwise
    // redraw only the rows whose highlight or value changed, and never on a timer.
    static bool        first = true;
    static uint16_t    last_hs;
    static uint8_t     full_frames;
    static menu_sync_t drawn;
    uint16_t           hs = (rgb_matrix_get_hue() << 8) | rgb_matrix_get_sat();
    if (first || hs != last_hs || repaint_overlay) {
        first           = false;
        repaint_overlay = false;
        last_hs         = hs;
        full_frames     = 2;
    }
    if (full_frames > 0) {
        full_frames--;
        drawn = v;
        for (uint8_t row = 0; row < ROW_COUNT; row++) draw_row(row, &v);
        return true;
    }
    if (drawn.open == v.open && drawn.cursor == v.cursor && drawn.autocorrect == v.autocorrect) return true;
    for (uint8_t row = 0; row < ROW_COUNT; row++) {
        bool was = drawn.open && drawn.cursor == row;
        bool now = v.open && v.cursor == row;
        if (was != now || (row == ROW_AUTO && drawn.autocorrect != v.autocorrect)) {
            draw_row(row, &v);
        }
    }
    drawn = v;
    return true;
}

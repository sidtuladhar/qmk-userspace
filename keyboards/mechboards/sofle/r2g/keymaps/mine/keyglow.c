// KeyGlow: music-reactive spectrum lighting driven over raw HID.
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The master (USB) half receives frames from the macOS app in
// raw_hid_receive_kb(), forwards them to the slave half over a split RPC, and
// both halves render the KEYGLOW_SPECTRUM effect from rgb_matrix_user.inc.
//
// This module owns keyboard_post_init_user() and housekeeping_task_user(). If
// the keymap ever needs those hooks too, rename these to keyglow_init() /
// keyglow_task() and call them from the keymap's versions.

#include QMK_KEYBOARD_H
#include "keyglow.h"
#include "split_util.h"
#include "transactions.h"
#include "via.h"
#ifdef DEBUG_PERF_ENABLE
#    include "debug_perf.h" // the keymap's perf debug page; times each main-loop pass
#endif

// Weak in quantum/via.c, but not declared in any header.
void raw_hid_receive_kb(uint8_t *data, uint8_t length);

keyglow_frame_t keyglow_frame;

static uint32_t kg_last_frame_time = 0;
static bool     kg_have_frame      = false;

// ---------------------------------------------------------------------------
// Freshness: both halves fade the bars on their own clocks once frames stop.
// ---------------------------------------------------------------------------
uint8_t keyglow_freshness(void) {
    if (!kg_have_frame) return 0;
    uint32_t elapsed = timer_elapsed32(kg_last_frame_time);
    if (elapsed <= KEYGLOW_HOLD_MS) return 255;
    elapsed -= KEYGLOW_HOLD_MS;
    if (elapsed >= KEYGLOW_FADE_MS) return 0;
    return 255 - (uint8_t)(elapsed * 255 / KEYGLOW_FADE_MS);
}

static void kg_store_frame(const keyglow_frame_t *frame) {
    keyglow_frame      = *frame;
    kg_last_frame_time = timer_read32();
    kg_have_frame      = true;
}

// ---------------------------------------------------------------------------
// Session state (master only). The previous lighting mode is saved on start
// and restored on stop or watchdog timeout; nothing here touches EEPROM.
// ---------------------------------------------------------------------------
typedef enum {
    KG_IDLE,       // not driving the lights
    KG_ACTIVE,     // our effect is selected and frames are flowing
    KG_OVERRIDDEN, // the user picked another effect mid-session; leave it alone
} kg_session_t;

static kg_session_t kg_session       = KG_IDLE;
static uint8_t      kg_saved_mode    = 0;
static bool         kg_saved_enabled = false;
static bool         kg_sync_pending  = false;
static uint32_t     kg_last_sync     = 0;
static uint32_t     kg_last_activity = 0; // last START or FRAME, for the watchdog

static void kg_start(void) {
    if (kg_session == KG_ACTIVE) return;
    kg_saved_mode    = rgb_matrix_get_mode();
    kg_saved_enabled = rgb_matrix_is_enabled();
    if (kg_saved_mode == RGB_MATRIX_CUSTOM_KEYGLOW_SPECTRUM) {
        // Booted straight into our effect (e.g. it was saved to EEPROM by an
        // RM_VAL* press mid-session); there is nothing sensible to go back to.
        kg_saved_mode = RGB_MATRIX_DEFAULT_MODE;
    }
    rgb_matrix_enable_noeeprom();
    rgb_matrix_mode_noeeprom(RGB_MATRIX_CUSTOM_KEYGLOW_SPECTRUM);
    kg_session       = KG_ACTIVE;
    kg_last_activity = timer_read32();
}

static void kg_stop(void) {
    if (kg_session == KG_ACTIVE) {
        rgb_matrix_mode_noeeprom(kg_saved_mode);
        if (!kg_saved_enabled) rgb_matrix_disable_noeeprom();
    }
    kg_session = KG_IDLE;
}

// ---------------------------------------------------------------------------
// Album art (master only). A new cover streams into kg_art_rx and is copied
// into keyglow_art_pixels only once every chunk has arrived and the END
// packet's CRC matches, so the screen always has a complete cover to draw and
// the old one stays up while the next track's streams in.
// ---------------------------------------------------------------------------
#if KEYGLOW_ART_SIDE > 0
uint8_t        keyglow_art_pixels[KEYGLOW_ART_SIDE * KEYGLOW_ART_SIDE * 2];
static uint8_t kg_art_rx[sizeof(keyglow_art_pixels)];
#    define KG_ART_BYTES sizeof(keyglow_art_pixels)
#    define KG_ART_CHUNKS ((KG_ART_BYTES + KEYGLOW_ART_CHUNK - 1) / KEYGLOW_ART_CHUNK)
static uint8_t kg_art_seen[(KG_ART_CHUNKS + 7) / 8]; // which chunks of the current id have arrived
#endif
static uint8_t          kg_art_id;     // id being received
static uint8_t          kg_art_shown;  // id of the complete cover in the buffer (0 = none)
static bool             kg_art_ok;     // the buffer holds a complete cover
static keyglow_player_t kg_player = KEYGLOW_PLAYER_STOPPED;
static uint8_t          kg_track_art;  // art id the host says this track goes with
static char             kg_title[KEYGLOW_TRACK_TEXT + 1];
static char             kg_artist[KEYGLOW_TRACK_TEXT + 1];
static uint16_t         kg_art_rev;
static uint32_t         kg_art_heard;  // last TRACK/ART packet

bool keyglow_art_ready(void) {
    return kg_art_ok && kg_art_shown != 0 && kg_art_shown == kg_track_art;
}
keyglow_player_t keyglow_player_state(void) {
    return kg_player;
}
const char *keyglow_track_title(void) {
    return kg_title;
}
const char *keyglow_track_artist(void) {
    return kg_artist;
}
uint16_t keyglow_art_revision(void) {
    return kg_art_rev;
}
uint32_t keyglow_art_idle_ms(void) {
    return timer_elapsed32(kg_art_heard);
}

#if KEYGLOW_ART_SIDE > 0
static uint16_t kg_crc16(const uint8_t *data, uint32_t len) { // CRC-16/CCITT-FALSE
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
        }
    }
    return crc;
}
#endif

static void kg_handle_art(uint8_t *data) {
    kg_art_heard = timer_read32();
#if KEYGLOW_ART_SIDE > 0
    uint8_t id = data[2];
    switch (data[1]) {
        case KG_ART_BEGIN:
            if (data[3] != KEYGLOW_ART_SIDE || data[4] != KEYGLOW_ART_SIDE || id == 0) {
                kg_art_id = 0; // wrong size for this build: ignore the rest
                break;
            }
            kg_art_id = id; // the shown cover stays valid until this one completes
            memset(kg_art_seen, 0, sizeof(kg_art_seen));
            break;

        case KG_ART_DATA: {
            if (id != kg_art_id || id == 0) break;
            uint16_t index  = data[3] | (uint16_t)data[4] << 8;
            uint32_t offset = (uint32_t)index * KEYGLOW_ART_CHUNK;
            if (index >= KG_ART_CHUNKS) break;
            uint32_t len = KG_ART_BYTES - offset < KEYGLOW_ART_CHUNK ? KG_ART_BYTES - offset : KEYGLOW_ART_CHUNK;
            memcpy(&kg_art_rx[offset], &data[5], len);
            kg_art_seen[index / 8] |= 1 << (index % 8);
            break;
        }

        case KG_ART_END: {
            if (id != kg_art_id || id == 0) break;
            for (uint16_t i = 0; i < KG_ART_CHUNKS; i++) {
                if (!(kg_art_seen[i / 8] & (1 << (i % 8)))) return; // a chunk went missing
            }
            uint16_t crc = data[3] | (uint16_t)data[4] << 8;
            if (kg_crc16(kg_art_rx, KG_ART_BYTES) != crc) return;
            memcpy(keyglow_art_pixels, kg_art_rx, KG_ART_BYTES);
            kg_art_ok    = true;
            kg_art_shown = id;
            kg_art_id    = 0; // done; late duplicates of this id are ignored
            kg_art_rev++;
            break;
        }
    }
#else
    (void)data;
#endif
}

static void kg_handle_track(uint8_t *data) {
    kg_art_heard          = timer_read32();
    keyglow_player_t state = data[2] <= KEYGLOW_PLAYER_PAUSED ? data[2] : KEYGLOW_PLAYER_STOPPED;
    uint8_t          field = data[4];
    uint8_t          part  = data[5];
    if (field > 1 || part > 1) return;

    char *text    = field == 0 ? kg_title : kg_artist;
    char  old[KEYGLOW_TRACK_PART];
    memcpy(old, &text[part * KEYGLOW_TRACK_PART], KEYGLOW_TRACK_PART);
    for (uint8_t i = 0; i < KEYGLOW_TRACK_PART; i++) {
        char c = data[6 + i];
        text[part * KEYGLOW_TRACK_PART + i] = (c >= 0x20 && c <= 0x7E) ? c : 0; // font is ASCII only
    }
    text[KEYGLOW_TRACK_TEXT] = 0;

    if (state != kg_player || data[3] != kg_track_art || memcmp(old, &text[part * KEYGLOW_TRACK_PART], KEYGLOW_TRACK_PART) != 0) {
        kg_player    = state;
        kg_track_art = data[3];
        kg_art_rev++;
    }
}

// ---------------------------------------------------------------------------
// Raw HID: VIA hands us every packet whose command ID it does not recognise.
// VIA echoes the buffer back afterwards, so never call raw_hid_send() here.
// ---------------------------------------------------------------------------
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    if (length <= KEYGLOW_BARS_OFFSET || data[0] != KEYGLOW_CMD_ID) {
        data[0] = id_unhandled;
        return;
    }

    switch (data[1]) {
        case KG_HELLO:
            data[2] = KEYGLOW_PROTOCOL_VERSION;
            data[3] = KEYGLOW_NUM_BANDS;
            data[4] = KEYGLOW_MAX_BARS;
            data[5] = KEYGLOW_ART_SIDE;
            break;

        case KG_ART_BEGIN:
        case KG_ART_DATA:
        case KG_ART_END:
            kg_handle_art(data);
            break;

        case KG_TRACK:
            kg_handle_track(data);
            break;

        case KG_START:
            kg_start();
            break;

        case KG_FRAME: {
            keyglow_frame_t frame;
            frame.seq     = data[2];
            frame.palette = data[3];
            frame.bars    = data[KEYGLOW_BARS_OFFSET] > KEYGLOW_MAX_BARS ? KEYGLOW_MAX_BARS : data[KEYGLOW_BARS_OFFSET];
            memcpy(frame.bands, &data[4], KEYGLOW_NUM_BANDS);
            kg_store_frame(&frame);
            kg_sync_pending  = true;
            kg_last_activity = timer_read32();
            // Frames after a watchdog timeout (Mac slept, keyboard replugged)
            // resume the session; frames after a manual override do not.
            if (kg_session == KG_IDLE) kg_start();
            break;
        }

        case KG_STOP:
            kg_stop();
            break;

        default:
            data[0] = id_unhandled;
            break;
    }
}

// ---------------------------------------------------------------------------
// Split relay: the slave half has no USB data, so copy frames across.
// ---------------------------------------------------------------------------
static void kg_sync_slave_handler(uint8_t in_len, const void *in, uint8_t out_len, void *out) {
    if (in_len == sizeof(keyglow_frame_t)) {
        kg_store_frame((const keyglow_frame_t *)in);
    }
}

void keyboard_post_init_user(void) {
    transaction_register_rpc(KEYGLOW_SYNC, kg_sync_slave_handler);
}

void housekeeping_task_user(void) {
#ifdef DEBUG_PERF_ENABLE
    debug_perf_loop(); // end of one main-loop pass (housekeeping is the last step in main())
#endif
    if (!is_keyboard_master()) return;

    if (kg_sync_pending && timer_elapsed32(kg_last_sync) >= KEYGLOW_SYNC_INTERVAL_MS) {
        if (is_transport_connected()) {
            transaction_rpc_send(KEYGLOW_SYNC, sizeof(keyglow_frame_t), &keyglow_frame);
        }
        kg_last_sync    = timer_read32();
        kg_sync_pending = false;
    }

    if (kg_session == KG_ACTIVE && rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_KEYGLOW_SPECTRUM) {
        kg_session = KG_OVERRIDDEN; // the user switched effects (e.g. RM_NEXT)
    }

    if (kg_session != KG_IDLE && timer_elapsed32(kg_last_activity) > KEYGLOW_WATCHDOG_MS) {
        kg_stop(); // host went quiet: put the old lighting back
    }
}

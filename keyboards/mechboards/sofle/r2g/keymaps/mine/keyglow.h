// KeyGlow: music-reactive spectrum lighting driven over raw HID.
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Wire protocol (host -> keyboard, one 32-byte raw HID output report):
//   [0]     KEYGLOW_CMD_ID (0x4B, 'K'); VIA leaves this ID unused
//   [1]     sub-command (keyglow_cmd_t)
//   [2]     sequence number (informational)
//   [3]     palette: hue offset added to the bass-red..treble-violet ramp
//   [4..19] KEYGLOW_NUM_BANDS band levels, 0-255, low to high frequency
//   [20]    bar count: 0 = smooth (16 bands blended across the board),
//           1..KEYGLOW_MAX_BARS = that many solid bars; bands[0..bars-1] used
// VIA echoes every packet back; for KG_HELLO the reply carries
//   [2] protocol version, [3] band count, [4] max bar count,
//   [5] album art side in pixels (v3; 0 = no art support).
//
// Album art (v3, master half only - it's drawn on the left/USB screen):
//   KG_ART_BEGIN [2] art id  [3] width  [4] height
//   KG_ART_DATA  [2] art id  [3..4] chunk index (LE)  [5..31] 27 pixel bytes
//   KG_ART_END   [2] art id  [3..4] CRC-16/CCITT-FALSE of the pixel bytes (LE)
//   KG_TRACK     [2] player state (0 stopped, 1 playing, 2 paused)
//                [3] art id the track goes with  [4] field (0 title, 1 artist)
//                [5] part (0/1)  [6..31] 26 ASCII bytes, NUL padded
// Pixels are RGB565 little-endian, row-major, KEYGLOW_ART_SIDE square.
//
// The keymap must define in config.h:
//   #define SPLIT_TRANSACTION_IDS_USER KEYGLOW_SYNC
// install.sh adds that line.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define KEYGLOW_CMD_ID 0x4B
#define KEYGLOW_PROTOCOL_VERSION 3
#define KEYGLOW_NUM_BANDS 16
// One bar per key column (6 per half). Divisors of 12 give equal-width bars.
#define KEYGLOW_MAX_BARS 12
#define KEYGLOW_BARS_OFFSET 20

// Restore the previous lighting if the host goes quiet for this long.
#define KEYGLOW_WATCHDOG_MS 2000
// Bars hold for this long after the last frame, then fade to the idle glow.
#define KEYGLOW_HOLD_MS 150
#define KEYGLOW_FADE_MS 600
// Minimum gap between frames forwarded to the slave half.
#define KEYGLOW_SYNC_INTERVAL_MS 8

typedef enum {
    KG_HELLO     = 0x00,
    KG_START     = 0x01,
    KG_FRAME     = 0x02,
    KG_STOP      = 0x03,
    KG_ART_BEGIN = 0x10,
    KG_ART_DATA  = 0x11,
    KG_ART_END   = 0x12,
    KG_TRACK     = 0x13,
} keyglow_cmd_t;

// Album art. Define KEYGLOW_ART_SIDE 0 in config.h to leave it out (saves 56 KB RAM:
// the shown cover plus a receive buffer).
#ifndef KEYGLOW_ART_SIDE
#    define KEYGLOW_ART_SIDE 120
#endif
#define KEYGLOW_ART_CHUNK 27
#define KEYGLOW_TRACK_PART 26
#define KEYGLOW_TRACK_TEXT (KEYGLOW_TRACK_PART * 2)

typedef enum {
    KEYGLOW_PLAYER_STOPPED = 0,
    KEYGLOW_PLAYER_PLAYING = 1,
    KEYGLOW_PLAYER_PAUSED  = 2,
} keyglow_player_t;

#if KEYGLOW_ART_SIDE > 0
// RGB565 little-endian, row-major; valid once keyglow_art_ready() is true.
extern uint8_t keyglow_art_pixels[KEYGLOW_ART_SIDE * KEYGLOW_ART_SIDE * 2];
#endif
bool             keyglow_art_ready(void);    // a complete, checksummed cover is in the buffer
keyglow_player_t keyglow_player_state(void); // as last reported by the host
const char      *keyglow_track_title(void);  // NUL-terminated, ASCII
const char      *keyglow_track_artist(void);
// Bumps whenever the cover, title, artist or player state changes; redraw when it moves.
uint16_t keyglow_art_revision(void);
// Milliseconds since the host last said anything about the track (any TRACK/ART packet).
uint32_t keyglow_art_idle_ms(void);

// Shared between halves via split RPC; must stay <= RPC_M2S_BUFFER_SIZE (32).
typedef struct __attribute__((packed)) {
    uint8_t seq;
    uint8_t palette;
    uint8_t bars; // 0 = smooth, else 1..KEYGLOW_MAX_BARS
    uint8_t bands[KEYGLOW_NUM_BANDS];
} keyglow_frame_t;

extern keyglow_frame_t keyglow_frame;

// 255 while frames are fresh, ramping to 0 once they go stale.
uint8_t keyglow_freshness(void);

// KeyGlow spectrum effect. Included by quantum/rgb_matrix when
// RGB_MATRIX_CUSTOM_USER = yes. Runs on both halves; each half renders only
// its own LEDs (RGB_MATRIX_USE_LIMITS) from the frame in keyglow_frame.
//
// Layout: bass on the far left key column, treble on the far right, with the
// 16 bands stretched across both halves (the gap between halves is skipped).
// Each band is a bar rising from the thumb row to the number row. Underglow
// glows with the band underneath it. Overall brightness and saturation follow
// the keyboard's own RGB settings (RM_VALU / RM_SATU).

RGB_MATRIX_EFFECT(KEYGLOW_SPECTRUM)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#    include "keyglow.h"

// Key LED x columns per half (keyboard.json): left 7..97, right 127..217.
#    define KG_LEFT_X_MIN 7
#    define KG_LEFT_X_MAX 97
#    define KG_RIGHT_X_MIN 127
#    define KG_RIGHT_X_MAX 217
// Key LED y rows: 0 (number row) .. 54 (thumb cluster).
#    define KG_Y_MAX 54
// Bar height that the tallest row needs, and how soft the top edge is.
#    define KG_BAR_TOP 200
#    define KG_BAR_SOFT 48
// Hue spread from bass to treble (0 red .. ~200 violet).
#    define KG_HUE_SPAN 200

// Position across the whole board, 0 (far left) .. 255 (far right).
static uint8_t kg_board_pos(uint8_t x) {
    if (x < 112) {
        if (x < KG_LEFT_X_MIN) x = KG_LEFT_X_MIN;
        if (x > KG_LEFT_X_MAX) x = KG_LEFT_X_MAX;
        return (uint8_t)((x - KG_LEFT_X_MIN) * 127 / (KG_LEFT_X_MAX - KG_LEFT_X_MIN));
    }
    if (x < KG_RIGHT_X_MIN) x = KG_RIGHT_X_MIN;
    if (x > KG_RIGHT_X_MAX) x = KG_RIGHT_X_MAX;
    return (uint8_t)(128 + (x - KG_RIGHT_X_MIN) * 127 / (KG_RIGHT_X_MAX - KG_RIGHT_X_MIN));
}

// Bar-mode position, 0..255. The six key columns of each half sit at the
// centres of 12 equal slots, so 12/6/4/3/2/1 bars always split on column
// boundaries. The right half mirrors the left; inner thumb keys belong to
// the innermost column.
#    define KG_COL_X_MAX 80
static uint8_t kg_column_pos(uint8_t x) {
    bool right = x >= 112;
    if (right) x = 224 - x;
    if (x < KG_LEFT_X_MIN) x = KG_LEFT_X_MIN;
    if (x > KG_COL_X_MAX) x = KG_COL_X_MAX;
    uint8_t p = (uint8_t)((32 + (uint16_t)(x - KG_LEFT_X_MIN) * 320 / (KG_COL_X_MAX - KG_LEFT_X_MIN)) / 3); // 10..117
    return right ? 255 - p : p;
}

// Band level at a board position, linearly interpolated between bands.
static uint8_t kg_level_at(uint8_t pos) {
    uint16_t b    = (uint16_t)pos * (KEYGLOW_NUM_BANDS - 1); // 0 .. 15*255
    uint8_t  idx  = b >> 8;
    uint8_t  frac = b & 0xFF;
    uint8_t  lo   = keyglow_frame.bands[idx];
    uint8_t  hi   = keyglow_frame.bands[idx + 1 < KEYGLOW_NUM_BANDS ? idx + 1 : idx];
    return (uint8_t)(lo + (((int16_t)hi - lo) * frac >> 8));
}

static bool KEYGLOW_SPECTRUM(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    uint8_t fresh = keyglow_freshness();
    hsv_t   base  = rgb_matrix_config.hsv;
    uint8_t bars  = keyglow_frame.bars;

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();

        uint8_t x = g_led_config.point[i].x;
        uint8_t raw;
        uint8_t hue_pos;
        if (bars == 0) {
            // Smooth: blend the 16 bands continuously across the board.
            hue_pos = kg_board_pos(x);
            raw     = kg_level_at(hue_pos);
        } else {
            // Bars: every LED in a bar shares one level and one colour. The
            // app has already grouped the 16 bands into bands[0..bars-1].
            uint8_t bar = (uint8_t)(((uint16_t)kg_column_pos(x) * bars) >> 8);
            raw         = keyglow_frame.bands[bar];
            hue_pos     = bars > 1 ? (uint8_t)((uint16_t)bar * 255 / (bars - 1)) : 128;
        }
        uint8_t level = scale8(raw, fresh);
        uint8_t brightness;

        if (HAS_FLAGS(g_led_config.flags[i], LED_FLAG_UNDERGLOW)) {
            // Underglow follows its band, with a faint glow while waiting for
            // the app so the effect never looks dead.
            uint8_t idle = (255 - fresh) >> 3;
            brightness   = level > idle ? level : idle;
        } else {
            // Keys: a bar of height `level`; this LED's height in that bar.
            uint8_t y      = g_led_config.point[i].y;
            uint8_t height = (uint8_t)((KG_Y_MAX - (y > KG_Y_MAX ? KG_Y_MAX : y)) * KG_BAR_TOP / KG_Y_MAX);
            int16_t over   = (int16_t)level - height;
            if (over <= 0) {
                brightness = 0;
            } else if (over >= KG_BAR_SOFT) {
                brightness = 255;
            } else {
                brightness = (uint8_t)(over * 255 / KG_BAR_SOFT); // soft top edge
            }
        }

        hsv_t hsv = {
            .h = (uint8_t)(keyglow_frame.palette + (uint16_t)hue_pos * KG_HUE_SPAN / 255),
            .s = base.s,
            .v = scale8(brightness, base.v),
        };
        rgb_t rgb = rgb_matrix_hsv_to_rgb(hsv);
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS

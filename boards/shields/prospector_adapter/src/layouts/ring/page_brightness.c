#include "page_brightness.h"
#include "ring_theme.h"
#include "display_colors.h"

#include <lvgl.h>
#include <prospector_brightness.h>

/* Number of fill dots used for brightness visualization. */
#define BRITE_DOTS 8
#define DOT_SPACING 24
#define SCR_CX 135
#define SCR_W  270

extern lv_font_t DINishCondensed_SemiBold_20;

/* ── Stored objects ─────────────────────────────────────────────────────── */

static lv_obj_t *s_header    = NULL;
static lv_obj_t *s_value     = NULL;
static lv_obj_t *s_hint      = NULL;
static lv_obj_t *s_fill_dots[BRITE_DOTS];

/* ── Helpers ────────────────────────────────────────────────────────────── */

static void refresh_fill_dots(uint8_t level) {
    /* level 0-100 → how many of the 8 dots are filled */
    int filled = (level * BRITE_DOTS + 50) / 100;
    for (int i = 0; i < BRITE_DOTS; i++) {
        if (!s_fill_dots[i]) continue;
        bool is_filled = (i < filled);
        lv_obj_set_style_bg_color(s_fill_dots[i],
            lv_color_hex(is_filled ? RING_COLOR_ACCENT : ring_color_track()),
            LV_PART_MAIN);
    }
}

static void refresh_value_label(uint8_t level) {
    if (s_value) {
        lv_label_set_text_fmt(s_value, "%d%%", level);
    }
}

/* ── Init ───────────────────────────────────────────────────────────────── */

void ring_page_brightness_init(lv_obj_t *parent) {
    /* Header label */
    s_header = lv_label_create(parent);
    lv_label_set_text(s_header, "BRIGHTNESS");
    lv_obj_set_style_text_font(s_header, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_header,
        lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(s_header, 2, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_header, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_header, SCR_W - 20);
    lv_obj_set_pos(s_header, 10, 50);

    /* Percentage value */
    s_value = lv_label_create(parent);
    lv_label_set_text(s_value, "50%");
    lv_obj_set_style_text_font(s_value, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_value,
        lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_value, SCR_W - 20);
    lv_obj_set_pos(s_value, 10, 74);

    /* 8 fill dots centered on screen */
    int total_w  = (BRITE_DOTS - 1) * DOT_SPACING;
    int start_x  = SCR_CX - total_w / 2;

    for (int i = 0; i < BRITE_DOTS; i++) {
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_pos(dot, start_x + i * DOT_SPACING - 5, 120);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(dot, 0, LV_PART_MAIN);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        s_fill_dots[i] = dot;
    }

    /* Hint label */
    s_hint = lv_label_create(parent);
    lv_label_set_text(s_hint, "< tap to adjust >");
    lv_obj_set_style_text_font(s_hint, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_hint,
        lv_color_hex(ring_color_subtext()), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_hint, SCR_W - 20);
    lv_obj_set_pos(s_hint, 10, 156);

    /* Draw initial state */
    uint8_t lvl = prospector_brightness_get_user_level();
    refresh_fill_dots(lvl);
    refresh_value_label(lvl);
}

/* ── Theme reapply ──────────────────────────────────────────────────────── */

void ring_page_brightness_apply_theme(void) {
    if (s_header) {
        lv_obj_set_style_text_color(s_header,
            lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    }
    if (s_value) {
        lv_obj_set_style_text_color(s_value,
            lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    }
    if (s_hint) {
        lv_obj_set_style_text_color(s_hint,
            lv_color_hex(ring_color_subtext()), LV_PART_MAIN);
    }
    /* Refresh fill dot colors using current brightness level */
    refresh_fill_dots(prospector_brightness_get_user_level());
}

/* ── Tap handler ────────────────────────────────────────────────────────── */

void ring_page_brightness_handle_tap(int16_t x) {
    if (x >= SCR_CX) {
        prospector_brightness_adjust_user_level(10);
    } else {
        prospector_brightness_adjust_user_level(-10);
    }
    uint8_t lvl = prospector_brightness_get_user_level();
    refresh_fill_dots(lvl);
    refresh_value_label(lvl);
}

/* ── Level sync ─────────────────────────────────────────────────────────── */

void ring_page_brightness_sync_level(void) {
    uint8_t lvl = prospector_brightness_get_user_level();
    refresh_fill_dots(lvl);
    refresh_value_label(lvl);
}

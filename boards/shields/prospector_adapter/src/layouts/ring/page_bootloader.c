#include "page_bootloader.h"
#include "ring_nav.h"
#include "ring_theme.h"
#include "display_colors.h"

#include <lvgl.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/sys/reboot.h>

/* ── Stored objects for theme reapply ──────────────────────────────────── */

static lv_obj_t *s_icon_circle  = NULL;
static lv_obj_t *s_icon_label   = NULL;
static lv_obj_t *s_title        = NULL;
static lv_obj_t *s_subtitle     = NULL;
static lv_obj_t *s_btn_cancel   = NULL;
static lv_obj_t *s_btn_cancel_l = NULL;
static lv_obj_t *s_btn_flash    = NULL;
static lv_obj_t *s_btn_flash_l  = NULL;

/* Screen center X for landscape display (≈140 for 280-wide screen). */
#define SCR_CX 135
#define SCR_W  270
#define BTN_Y 136
#define BTN_H 30
#define BTN_CANCEL_X 20
#define BTN_FLASH_X 142
#define BTN_W 108

static lv_obj_t *make_button(lv_obj_t *parent, int16_t x, int16_t y,
                              int16_t w, int16_t h) {
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return btn;
}

/* ── Init ───────────────────────────────────────────────────────────────── */

void ring_page_bootloader_init(lv_obj_t *parent) {
    /* Warning icon: accent circle with "!" */
    s_icon_circle = lv_obj_create(parent);
    lv_obj_set_size(s_icon_circle, 32, 32);
    lv_obj_set_pos(s_icon_circle, SCR_CX - 16, 38);
    lv_obj_set_style_radius(s_icon_circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_icon_circle, lv_color_hex(RING_COLOR_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_icon_circle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_icon_circle, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_icon_circle, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_icon_circle, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_icon_circle, LV_OBJ_FLAG_SCROLLABLE);

    s_icon_label = lv_label_create(s_icon_circle);
    lv_label_set_text(s_icon_label, "!");
    lv_obj_set_style_text_font(s_icon_label, &lv_font_montserrat_14, LV_PART_MAIN);
    /* "!" on accent bg: always dark (bg color) */
    lv_obj_set_style_text_color(s_icon_label,
        lv_color_hex(ring_color_bg()), LV_PART_MAIN);
    lv_obj_align(s_icon_label, LV_ALIGN_CENTER, 0, 0);

    /* Title */
    s_title = lv_label_create(parent);
    lv_label_set_text(s_title, "Enter Bootloader?");
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_title,
        lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_title, SCR_W - 20);
    lv_obj_set_pos(s_title, 10, 82);

    /* Subtitle */
    s_subtitle = lv_label_create(parent);
    lv_label_set_text(s_subtitle, "Keyboard will disconnect.");
    lv_obj_set_style_text_font(s_subtitle, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_subtitle,
        lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_subtitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_subtitle, SCR_W - 20);
    lv_obj_set_pos(s_subtitle, 10, 104);

    /* Cancel button (left, outline style) */
    s_btn_cancel = make_button(parent, 20, 136, 108, 30);
    lv_obj_set_style_bg_opa(s_btn_cancel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_btn_cancel,
        lv_color_hex(ring_color_track()), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_btn_cancel, 1, LV_PART_MAIN);

    s_btn_cancel_l = lv_label_create(s_btn_cancel);
    lv_label_set_text(s_btn_cancel_l, "Cancel");
    lv_obj_set_style_text_font(s_btn_cancel_l, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_btn_cancel_l,
        lv_color_hex(ring_color_cancel_text()), LV_PART_MAIN);
    lv_obj_align(s_btn_cancel_l, LV_ALIGN_CENTER, 0, 0);

    /* Flash button (right, accent fill) */
    s_btn_flash = make_button(parent, 142, 136, 108, 30);
    lv_obj_set_style_bg_color(s_btn_flash,
        lv_color_hex(RING_COLOR_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_btn_flash, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_btn_flash, 0, LV_PART_MAIN);

    s_btn_flash_l = lv_label_create(s_btn_flash);
    lv_label_set_text(s_btn_flash_l, "Flash");
    lv_obj_set_style_text_font(s_btn_flash_l, &lv_font_montserrat_12, LV_PART_MAIN);
    /* Flash label: always dark bg color on accent background */
    lv_obj_set_style_text_color(s_btn_flash_l,
        lv_color_hex(0x22282Cu), LV_PART_MAIN);
    lv_obj_align(s_btn_flash_l, LV_ALIGN_CENTER, 0, 0);
}

/* ── Theme reapply ──────────────────────────────────────────────────────── */

void ring_page_bootloader_apply_theme(void) {
    if (s_icon_label) {
        lv_obj_set_style_text_color(s_icon_label,
            lv_color_hex(ring_color_bg()), LV_PART_MAIN);
    }
    if (s_title) {
        lv_obj_set_style_text_color(s_title,
            lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    }
    if (s_subtitle) {
        lv_obj_set_style_text_color(s_subtitle,
            lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    }
    if (s_btn_cancel) {
        lv_obj_set_style_border_color(s_btn_cancel,
            lv_color_hex(ring_color_track()), LV_PART_MAIN);
    }
    if (s_btn_cancel_l) {
        lv_obj_set_style_text_color(s_btn_cancel_l,
            lv_color_hex(ring_color_cancel_text()), LV_PART_MAIN);
    }
}

/* ── Tap handler ────────────────────────────────────────────────────────── */

static void do_bootloader_async(void *data) {
    ARG_UNUSED(data);

    if (bootmode_set(BOOT_MODE_TYPE_BOOTLOADER) == 0) {
        sys_reboot(SYS_REBOOT_WARM);
    }
}

static bool point_in_rect(int16_t x, int16_t y, int16_t rx, int16_t ry,
                          int16_t rw, int16_t rh) {
    return x >= rx && x < (rx + rw) && y >= ry && y < (ry + rh);
}

void ring_page_bootloader_handle_tap(int16_t x, int16_t y) {
    if (point_in_rect(x, y, BTN_FLASH_X, BTN_Y, BTN_W, BTN_H)) {
        lv_async_call(do_bootloader_async, NULL);
    } else if (point_in_rect(x, y, BTN_CANCEL_X, BTN_Y, BTN_W, BTN_H)) {
        ring_nav_swipe_left();
    }
}

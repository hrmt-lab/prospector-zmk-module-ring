#include <lvgl.h>

#include "battery_rings.h"
#include "brightness_info.h"
#include "keys_info.h"
#include "modifier_chips.h"
#include "uptime_info.h"
#include "display_colors.h"
#include "ring_theme.h"

static struct zmk_widget_battery_rings battery_rings_widget;
static struct zmk_widget_brightness_info brightness_info_widget;
static struct zmk_widget_keys_info keys_info_widget;
static struct zmk_widget_modifier_chips modifier_chips_widget;
static struct zmk_widget_uptime_info uptime_info_widget;

/* Screen-level objects stored for theme reapply. */
static lv_obj_t *s_screen   = NULL;
static lv_obj_t *s_kb_label = NULL;
static lv_obj_t *s_divider  = NULL;

lv_obj_t *zmk_display_status_screen(void) {
    s_screen = lv_obj_create(NULL);
    lv_obj_t *screen = s_screen;
    lv_obj_set_style_bg_color(screen, lv_color_hex(ring_color_bg()), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_t *content = screen;

    // Keyboard name (header left)
#ifdef CONFIG_ZMK_KEYBOARD_NAME
    const char *kb_name = CONFIG_ZMK_KEYBOARD_NAME;
#else
    const char *kb_name = "PROSPECTOR";
#endif
    s_kb_label = lv_label_create(content);
    lv_obj_t *kb_label = s_kb_label;
    lv_label_set_text(kb_label, kb_name);
    lv_obj_set_style_text_font(kb_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(kb_label, lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    lv_obj_set_pos(kb_label, 18, 11);

    // Vertical divider (x=190, y=36-208)
    s_divider = lv_obj_create(content);
    lv_obj_t *divider = s_divider;
    lv_obj_set_size(divider, 1, 172);
    lv_obj_set_pos(divider, 190, 36);
    lv_obj_set_style_bg_color(divider, lv_color_hex(ring_color_text_off()), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);

    // Widgets
    zmk_widget_uptime_info_init(&uptime_info_widget, screen);
    zmk_widget_battery_rings_init(&battery_rings_widget, content);
    zmk_widget_brightness_info_init(&brightness_info_widget, content);
    zmk_widget_modifier_chips_init(&modifier_chips_widget, content);
    zmk_widget_keys_info_init(&keys_info_widget, content);

    return screen;
}

/* Show/hide Main-only screen decoration (the vertical divider at x=190).
 * Hidden while the AI Usage screen is shown. */
void ring_status_screen_set_main_decor_visible(bool visible) {
    if (!s_divider) {
        return;
    }
    if (visible) {
        lv_obj_clear_flag(s_divider, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_divider, LV_OBJ_FLAG_HIDDEN);
    }
}

void ring_status_screen_apply_theme(void) {
    if (s_screen) {
        lv_obj_set_style_bg_color(s_screen,
            lv_color_hex(ring_color_bg()), LV_PART_MAIN);
    }
    if (s_kb_label) {
        lv_obj_set_style_text_color(s_kb_label,
            lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    }
    if (s_divider) {
        lv_obj_set_style_bg_color(s_divider,
            lv_color_hex(ring_color_text_off()), LV_PART_MAIN);
    }
}

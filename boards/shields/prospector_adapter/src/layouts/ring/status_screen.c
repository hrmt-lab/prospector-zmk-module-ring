#include <lvgl.h>

#include "battery_rings.h"
#include "modifier_chips.h"
#include "output_info.h"
#include "display_colors.h"

extern lv_font_t DINishCondensed_SemiBold_20;

static struct zmk_widget_battery_rings battery_rings_widget;
static struct zmk_widget_modifier_chips modifier_chips_widget;
static struct zmk_widget_output_info    output_info_widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(RING_COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);

    // Keyboard name (header left)
#ifdef CONFIG_ZMK_KEYBOARD_NAME
    const char *kb_name = CONFIG_ZMK_KEYBOARD_NAME;
#else
    const char *kb_name = "PROSPECTOR";
#endif
    lv_obj_t *kb_label = lv_label_create(screen);
    lv_label_set_text(kb_label, kb_name);
    lv_obj_set_style_text_font(kb_label, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(kb_label, lv_color_hex(RING_COLOR_TEXT_PRI), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(kb_label, 1, LV_PART_MAIN);
    lv_obj_set_pos(kb_label, 14, 9);

    // Vertical divider (x=190, y=34-210)
    lv_obj_t *divider = lv_obj_create(screen);
    lv_obj_set_size(divider, 1, 176);
    lv_obj_set_pos(divider, 190, 34);
    lv_obj_set_style_bg_color(divider, lv_color_hex(RING_COLOR_TRACK), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);

    // Widgets
    zmk_widget_battery_rings_init(&battery_rings_widget, screen);
    zmk_widget_modifier_chips_init(&modifier_chips_widget, screen);
    zmk_widget_output_info_init(&output_info_widget, screen);

    return screen;
}

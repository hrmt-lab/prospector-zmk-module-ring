#include "brightness_info.h"

#include <stdio.h>
#include <prospector_brightness.h>

#include "display_colors.h"
#include "ring_theme.h"

#define BRIGHTNESS_X 18
#define BRIGHTNESS_Y 210
#define ICON_SIZE    8
#define VALUE_X      (BRIGHTNESS_X + 14)
#define VALUE_W      42

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void format_brightness(char *buf, size_t len) {
    snprintf(buf, len, "%u%%", prospector_brightness_get_user_level());
}

static void apply_widget_theme(struct zmk_widget_brightness_info *widget) {
    if (widget->icon) {
        lv_obj_set_style_bg_color(widget->icon, lv_color_hex(RING_COLOR_ACCENT), LV_PART_MAIN);
        lv_obj_set_style_border_color(widget->icon, lv_color_hex(ring_color_bg()), LV_PART_MAIN);
    }
    if (widget->value) {
        lv_obj_set_style_text_color(widget->value, lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    }
}

static void refresh_widget(struct zmk_widget_brightness_info *widget) {
    if (!widget->value) {
        return;
    }

    char buf[8];
    format_brightness(buf, sizeof(buf));
    lv_label_set_text(widget->value, buf);
}

static void refresh_async_cb(void *data) {
    ARG_UNUSED(data);

    struct zmk_widget_brightness_info *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        refresh_widget(widget);
    }
}

int zmk_widget_brightness_info_init(struct zmk_widget_brightness_info *widget, lv_obj_t *parent) {
    widget->obj = parent;

    widget->icon = lv_obj_create(parent);
    lv_obj_set_size(widget->icon, ICON_SIZE, ICON_SIZE);
    lv_obj_set_pos(widget->icon, BRIGHTNESS_X, BRIGHTNESS_Y + 5);
    lv_obj_set_style_radius(widget->icon, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->icon, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->icon, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(widget->icon, 0, LV_PART_MAIN);
    lv_obj_clear_flag(widget->icon, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    widget->value = lv_label_create(parent);
    lv_obj_set_style_text_font(widget->value, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_align(widget->value, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_width(widget->value, VALUE_W);
    lv_obj_set_pos(widget->value, VALUE_X, BRIGHTNESS_Y);

    sys_slist_append(&widgets, &widget->node);
    apply_widget_theme(widget);
    refresh_widget(widget);

    return 0;
}

lv_obj_t *zmk_widget_brightness_info_obj(struct zmk_widget_brightness_info *widget) {
    return widget->obj;
}

void ring_brightness_info_refresh(void) {
    lv_async_call(refresh_async_cb, NULL);
}

void ring_brightness_info_apply_theme(void) {
    struct zmk_widget_brightness_info *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        apply_widget_theme(widget);
    }
}

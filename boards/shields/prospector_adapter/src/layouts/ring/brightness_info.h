#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_brightness_info {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *icon;
    lv_obj_t *value;
};

int zmk_widget_brightness_info_init(struct zmk_widget_brightness_info *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_brightness_info_obj(struct zmk_widget_brightness_info *widget);
void ring_brightness_info_refresh(void);
void ring_brightness_info_apply_theme(void);

/* Show/hide the brightness icon + value (hidden on the AI Usage screen). */
void ring_brightness_info_set_visible(bool visible);

#pragma once

#include <lvgl.h>

struct zmk_widget_uptime_info {
    lv_obj_t *obj;
};

int zmk_widget_uptime_info_init(struct zmk_widget_uptime_info *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_uptime_info_obj(struct zmk_widget_uptime_info *widget);
void ring_uptime_info_apply_theme(void);

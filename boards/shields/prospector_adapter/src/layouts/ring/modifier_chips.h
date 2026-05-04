#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_modifier_chips {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *mod_chips[4];   // [0]=CTRL [1]=SHFT [2]=ALT [3]=GUI
    lv_obj_t *state_chips[2]; // [0]=CAPS [1]=IME
};

int zmk_widget_modifier_chips_init(struct zmk_widget_modifier_chips *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_modifier_chips_obj(struct zmk_widget_modifier_chips *widget);
void ring_modifier_chips_apply_theme(void);

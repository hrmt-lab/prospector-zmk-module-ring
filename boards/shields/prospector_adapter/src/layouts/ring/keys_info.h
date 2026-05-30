#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

struct zmk_widget_keys_info {
    sys_snode_t node;
    lv_obj_t *obj;
#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
    lv_obj_t *last_value;
#endif
    lv_obj_t *keys_value;
};

int zmk_widget_keys_info_init(struct zmk_widget_keys_info *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_keys_info_obj(struct zmk_widget_keys_info *widget);
void ring_keys_info_apply_theme(void);

/* Show/hide the LAST and KEYS labels (hidden on the AI Usage screen). */
void ring_keys_info_set_visible(bool visible);

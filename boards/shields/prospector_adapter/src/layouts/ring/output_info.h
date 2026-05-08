#pragma once

/* Legacy RING output panel.
 *
 * This widget used to render OUT (USB/BLE), BLE profile, dongle battery,
 * and KEYS in one block. RING currently omits OUT/BLE/dongle-battery for a
 * simpler USB-first layout; KEYS/LAST live in keys_info.c instead.
 * The file is intentionally kept for possible future reference/revival, but
 * output_info.c is excluded from the RING build in CMakeLists.txt.
 */

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_output_info {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *out_proto;    // "BLE" / "USB"
    lv_obj_t *out_profile;  // "1"-"5" (BLE only, accent color)
    lv_obj_t *keys_value;   // "8,247" etc.
    lv_obj_t *dongle_group; // header-area dongle battery container
    lv_obj_t *dongle_value; // "96" etc.
};

int zmk_widget_output_info_init(struct zmk_widget_output_info *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_output_info_obj(struct zmk_widget_output_info *widget);
void ring_output_info_apply_theme(void);

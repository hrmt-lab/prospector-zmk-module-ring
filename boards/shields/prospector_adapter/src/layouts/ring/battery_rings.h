#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/ble.h>

#ifndef RING_PERIPHERAL_COUNT
#define RING_PERIPHERAL_COUNT ZMK_SPLIT_BLE_PERIPHERAL_COUNT
#endif

struct zmk_widget_battery_rings {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *arcs[RING_PERIPHERAL_COUNT];
    lv_obj_t *layer_name;
    lv_obj_t *batt_dots[RING_PERIPHERAL_COUNT];
    lv_obj_t *batt_values[RING_PERIPHERAL_COUNT];
};

int zmk_widget_battery_rings_init(struct zmk_widget_battery_rings *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_battery_rings_obj(struct zmk_widget_battery_rings *widget);

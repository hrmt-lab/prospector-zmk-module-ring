#pragma once

#include <lvgl.h>
#include <stdint.h>

void ring_page_brightness_init(lv_obj_t *parent);
void ring_page_brightness_apply_theme(void);
void ring_page_brightness_handle_tap(int16_t x);
/* Sync displayed value with current hardware brightness level. */
void ring_page_brightness_sync_level(void);

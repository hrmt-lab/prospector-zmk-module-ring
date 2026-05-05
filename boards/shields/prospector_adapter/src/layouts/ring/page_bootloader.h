#pragma once

#include <lvgl.h>
#include <stdint.h>

void ring_page_bootloader_init(lv_obj_t *parent);
void ring_page_bootloader_apply_theme(void);
void ring_page_bootloader_handle_tap(int16_t x);

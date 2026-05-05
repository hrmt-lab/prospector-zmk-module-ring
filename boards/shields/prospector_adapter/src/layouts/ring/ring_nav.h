#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    RING_PAGE_MAIN = 0,
    RING_PAGE_BOOTLOADER,
    RING_PAGE_BRIGHTNESS,
} ring_page_t;

/* Call from zmk_display_status_screen() after creating page_main container.
 * Creates bootloader/brightness containers and page-dot indicators on screen. */
void ring_nav_init(lv_obj_t *screen, lv_obj_t *page_main);

lv_obj_t *ring_nav_get_bootloader_page(void);
lv_obj_t *ring_nav_get_brightness_page(void);

ring_page_t ring_nav_get_page(void);

/* Called from ring_touch.c */
void ring_nav_swipe_left(void);
void ring_nav_swipe_right(void);
void ring_nav_handle_tap(int16_t x, int16_t y);

/* Called from ring_theme.c */
void ring_nav_apply_theme(void);

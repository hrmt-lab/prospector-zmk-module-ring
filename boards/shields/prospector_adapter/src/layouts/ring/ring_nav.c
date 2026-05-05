#include "ring_nav.h"
#include "ring_theme.h"
#include "display_colors.h"
#include "page_bootloader.h"
#include "page_brightness.h"

#include <lvgl.h>

/* ── State ─────────────────────────────────────────────────────────────── */

static ring_page_t   s_current_page    = RING_PAGE_MAIN;
static lv_obj_t     *s_page_main       = NULL;
static lv_obj_t     *s_page_bootloader = NULL;
static lv_obj_t     *s_page_brightness = NULL;

/* Page indicator dots — on screen level, shared across pages.
 * Index: 0=Bootloader, 1=Main, 2=Brightness */
static lv_obj_t *s_dots[3] = {NULL, NULL, NULL};

/* Page-dot layout: centered at RING_CENTER_X=96, y just below ring bottom.
 * With outer radius 70 and center_y 124: ring bottom = 194, dots at y=204. */
#define DOT_CY        204
#define DOT_CX_BOOT   80
#define DOT_CX_MAIN   96
#define DOT_CX_BRIGHT 112
#define DOT_R_ACTIVE  4
#define DOT_R_INACTIVE 3

/* ── Helpers ────────────────────────────────────────────────────────────── */

static lv_obj_t *make_page_container(lv_obj_t *screen) {
    lv_obj_t *page = lv_obj_create(screen);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(page, 0, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(page, 0, LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

static lv_obj_t *make_dot(lv_obj_t *screen, int16_t cx, int16_t cy, uint8_t r,
                           uint32_t color) {
    lv_obj_t *dot = lv_obj_create(screen);
    lv_obj_set_size(dot, r * 2, r * 2);
    lv_obj_set_pos(dot, cx - r, cy - r);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(dot, 0, LV_PART_MAIN);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    return dot;
}

static void update_dots(void) {
    /* dot indices: 0=boot, 1=main, 2=bright */
    static const ring_page_t dot_page[3] = {
        RING_PAGE_BOOTLOADER, RING_PAGE_MAIN, RING_PAGE_BRIGHTNESS
    };
    static const int16_t dot_cx[3] = {DOT_CX_BOOT, DOT_CX_MAIN, DOT_CX_BRIGHT};

    for (int i = 0; i < 3; i++) {
        if (!s_dots[i]) continue;
        bool active = (dot_page[i] == s_current_page);
        uint8_t r  = active ? DOT_R_ACTIVE : DOT_R_INACTIVE;
        uint32_t c = active ? ring_color_page_dot_active() : ring_color_page_dot_inactive();

        /* Resize by repositioning with new radius */
        lv_obj_set_size(s_dots[i], r * 2, r * 2);
        lv_obj_set_pos(s_dots[i], dot_cx[i] - r, DOT_CY - r);
        lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(c), LV_PART_MAIN);
    }
}

/* ── Page switch (must be called on LVGL thread) ────────────────────────── */

typedef struct { ring_page_t target; } nav_async_data_t;
static nav_async_data_t s_nav_data;

static void do_page_switch_async(void *data) {
    ring_page_t target = ((nav_async_data_t *)data)->target;
    if (target == s_current_page) return;

    /* On entering brightness page, sync display to current hardware level. */
    if (target == RING_PAGE_BRIGHTNESS) {
        ring_page_brightness_sync_level();
    }

    /* Hide current page */
    lv_obj_t *cur = (s_current_page == RING_PAGE_MAIN)       ? s_page_main
                  : (s_current_page == RING_PAGE_BOOTLOADER)  ? s_page_bootloader
                                                              : s_page_brightness;
    if (cur) lv_obj_add_flag(cur, LV_OBJ_FLAG_HIDDEN);

    s_current_page = target;

    /* Show new page */
    lv_obj_t *nxt = (target == RING_PAGE_MAIN)       ? s_page_main
                  : (target == RING_PAGE_BOOTLOADER)  ? s_page_bootloader
                                                     : s_page_brightness;
    if (nxt) lv_obj_remove_flag(nxt, LV_OBJ_FLAG_HIDDEN);

    update_dots();
}

static void go_to_page(ring_page_t target) {
    s_nav_data.target = target;
    lv_async_call(do_page_switch_async, &s_nav_data);
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void ring_nav_init(lv_obj_t *screen, lv_obj_t *page_main) {
    s_page_main = page_main;

    s_page_bootloader = make_page_container(screen);
    lv_obj_add_flag(s_page_bootloader, LV_OBJ_FLAG_HIDDEN);

    s_page_brightness = make_page_container(screen);
    lv_obj_add_flag(s_page_brightness, LV_OBJ_FLAG_HIDDEN);

    /* Page dots sit directly on screen so they're always visible */
    s_dots[0] = make_dot(screen, DOT_CX_BOOT,   DOT_CY, DOT_R_INACTIVE,
                          ring_color_page_dot_inactive());
    s_dots[1] = make_dot(screen, DOT_CX_MAIN,   DOT_CY, DOT_R_ACTIVE,
                          ring_color_page_dot_active());
    s_dots[2] = make_dot(screen, DOT_CX_BRIGHT, DOT_CY, DOT_R_INACTIVE,
                          ring_color_page_dot_inactive());

    s_current_page = RING_PAGE_MAIN;
}

lv_obj_t *ring_nav_get_bootloader_page(void) { return s_page_bootloader; }
lv_obj_t *ring_nav_get_brightness_page(void)  { return s_page_brightness;  }
ring_page_t ring_nav_get_page(void)           { return s_current_page;    }

void ring_nav_swipe_left(void) {
    if (s_current_page == RING_PAGE_BOOTLOADER) {
        go_to_page(RING_PAGE_MAIN);
    } else if (s_current_page == RING_PAGE_MAIN) {
        go_to_page(RING_PAGE_BRIGHTNESS);
    }
}

void ring_nav_swipe_right(void) {
    if (s_current_page == RING_PAGE_MAIN) {
        go_to_page(RING_PAGE_BOOTLOADER);
    } else if (s_current_page == RING_PAGE_BRIGHTNESS) {
        go_to_page(RING_PAGE_MAIN);
    }
}

void ring_nav_handle_tap(int16_t x, int16_t y) {
    if (s_current_page == RING_PAGE_BOOTLOADER) {
        ring_page_bootloader_handle_tap(x, y);
    } else if (s_current_page == RING_PAGE_BRIGHTNESS) {
        ring_page_brightness_handle_tap(x);
    }
}

void ring_nav_apply_theme(void) {
    update_dots();
}

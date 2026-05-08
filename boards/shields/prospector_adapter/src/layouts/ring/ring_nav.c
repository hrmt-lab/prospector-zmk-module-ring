#include "ring_nav.h"
#include "ring_theme.h"
#include "display_colors.h"
#include "page_bootloader.h"
#include "page_brightness.h"

#include <lvgl.h>

/* ── State ─────────────────────────────────────────────────────────────── */

/* Page order also drives swipe direction and indicator position. */
#define PAGE_COUNT 3
static const ring_page_t PAGE_ORDER[PAGE_COUNT] = {
    RING_PAGE_BOOTLOADER, RING_PAGE_MAIN, RING_PAGE_BRIGHTNESS
};

static ring_page_t   s_current_page    = RING_PAGE_MAIN;
static lv_obj_t     *s_pages[PAGE_COUNT] = {NULL, NULL, NULL};
static lv_obj_t     *s_dots[PAGE_COUNT] = {NULL, NULL, NULL};

/* Page-dot layout: centered just below the battery rings. */
#define DOT_CY        204
#define DOT_CENTER_X  96
#define DOT_SPACING   12
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

static int page_index(ring_page_t page) {
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (PAGE_ORDER[i] == page) {
            return i;
        }
    }
    return -1;
}

static lv_obj_t *page_obj_for(ring_page_t page) {
    int idx = page_index(page);
    if (idx < 0) {
        return NULL;
    }
    return s_pages[idx];
}

static void update_dots(void) {
    int active_idx = page_index(s_current_page);

    for (int i = 0; i < PAGE_COUNT; i++) {
        if (!s_dots[i]) {
            continue;
        }

        bool active = (i == active_idx);
        uint8_t r = active ? DOT_R_ACTIVE : DOT_R_INACTIVE;
        uint32_t color = active ? RING_COLOR_ACCENT : ring_color_page_dot_inactive();
        int16_t cx = DOT_CENTER_X + ((2 * i + 1 - PAGE_COUNT) * DOT_SPACING) / 2;

        lv_obj_set_size(s_dots[i], r * 2, r * 2);
        lv_obj_set_pos(s_dots[i], cx - r, DOT_CY - r);
        lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(color), LV_PART_MAIN);
    }
}

/* ── Page switch (must be called on LVGL thread) ────────────────────────── */

static void do_page_switch_async(void *data) {
    ring_page_t target = (ring_page_t)(uintptr_t)data;
    if (target == s_current_page) return;

    /* On entering brightness page, sync display to current hardware level. */
    if (target == RING_PAGE_BRIGHTNESS) {
        ring_page_brightness_sync_level();
    }

    /* Hide current page */
    lv_obj_t *cur = page_obj_for(s_current_page);
    if (cur) lv_obj_add_flag(cur, LV_OBJ_FLAG_HIDDEN);

    s_current_page = target;

    /* Show new page */
    lv_obj_t *nxt = page_obj_for(target);
    if (nxt) lv_obj_remove_flag(nxt, LV_OBJ_FLAG_HIDDEN);

    update_dots();
}

static void go_to_page(ring_page_t target) {
    lv_async_call(do_page_switch_async, (void *)(uintptr_t)target);
}

static void go_relative(int delta) {
    int idx = page_index(s_current_page);
    int target_idx = idx + delta;

    if (idx < 0 || target_idx < 0 || target_idx >= PAGE_COUNT) {
        return;
    }

    go_to_page(PAGE_ORDER[target_idx]);
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void ring_nav_init(lv_obj_t *screen, lv_obj_t *page_main) {
    s_pages[page_index(RING_PAGE_MAIN)] = page_main;

    s_pages[page_index(RING_PAGE_BOOTLOADER)] = make_page_container(screen);
    lv_obj_add_flag(s_pages[page_index(RING_PAGE_BOOTLOADER)], LV_OBJ_FLAG_HIDDEN);

    s_pages[page_index(RING_PAGE_BRIGHTNESS)] = make_page_container(screen);
    lv_obj_add_flag(s_pages[page_index(RING_PAGE_BRIGHTNESS)], LV_OBJ_FLAG_HIDDEN);

    s_current_page = RING_PAGE_MAIN;

    for (int i = 0; i < PAGE_COUNT; i++) {
        int16_t cx = DOT_CENTER_X + ((2 * i + 1 - PAGE_COUNT) * DOT_SPACING) / 2;
        bool active = (i == page_index(s_current_page));
        s_dots[i] = make_dot(screen, cx, DOT_CY,
                             active ? DOT_R_ACTIVE : DOT_R_INACTIVE,
                             active ? ring_color_page_dot_active()
                                    : ring_color_page_dot_inactive());
    }

    update_dots();
}

lv_obj_t *ring_nav_get_bootloader_page(void) { return page_obj_for(RING_PAGE_BOOTLOADER); }
lv_obj_t *ring_nav_get_brightness_page(void)  { return page_obj_for(RING_PAGE_BRIGHTNESS); }
ring_page_t ring_nav_get_page(void)           { return s_current_page;    }

void ring_nav_swipe_left(void) {
    go_relative(1);
}

void ring_nav_swipe_right(void) {
    go_relative(-1);
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

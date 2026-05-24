#include "uptime_info.h"

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zmk/display.h>

#if __has_include(<hitsuki46/raw_hid_time_sync.h>)
#include <hitsuki46/raw_hid_time_sync.h>
#define RING_HAS_HITSUKI46_TIME_SYNC 1
#else
#define RING_HAS_HITSUKI46_TIME_SYNC 0
#endif

#include "ring_theme.h"

#define UPTIME_X      198
#define UPTIME_Y      10
#define UPTIME_W      64
#define UPTIME_PERIOD K_MINUTES(1)

#define TIME_SYNC_X      112
#define TIME_SYNC_W      150
#define TIME_SYNC_PERIOD K_SECONDS(1)
#define TIME_SYNC_COLOR  0x000000

static lv_obj_t *s_uptime_label = NULL;
static struct k_work_delayable uptime_work;
static bool s_showing_time_sync = false;

static void format_uptime(char *buf, size_t len) {
    uint32_t total_minutes = k_uptime_get() / 60000;
    uint32_t hours = total_minutes / 60;
    uint32_t minutes = total_minutes % 60;

    if (hours < 100) {
        snprintf(buf, len, "UP  %u:%02u", hours, minutes);
    } else {
        snprintf(buf, len, "UP  %uh", hours);
    }
}

static bool format_time_sync(char *buf, size_t len) {
#if RING_HAS_HITSUKI46_TIME_SYNC
    return hitsuki46_raw_hid_time_sync_format(buf, len);
#else
    ARG_UNUSED(buf);
    ARG_UNUSED(len);
    return false;
#endif
}

static bool time_sync_wants_seconds(void) {
#if RING_HAS_HITSUKI46_TIME_SYNC
    return hitsuki46_raw_hid_time_sync_wants_seconds();
#else
    return false;
#endif
}

static void apply_display_mode(bool showing_time_sync) {
    if (!s_uptime_label) {
        return;
    }

    s_showing_time_sync = showing_time_sync;

    if (showing_time_sync) {
        lv_obj_set_style_text_font(s_uptime_label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_uptime_label, lv_color_hex(TIME_SYNC_COLOR), LV_PART_MAIN);
        lv_obj_set_width(s_uptime_label, TIME_SYNC_W);
        lv_obj_set_pos(s_uptime_label, TIME_SYNC_X, UPTIME_Y);
    } else {
        lv_obj_set_style_text_font(s_uptime_label, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_uptime_label, lv_color_hex(ring_color_text_off()), LV_PART_MAIN);
        lv_obj_set_width(s_uptime_label, UPTIME_W);
        lv_obj_set_pos(s_uptime_label, UPTIME_X, UPTIME_Y);
    }
}

static void uptime_update_cb(struct k_work *work) {
    ARG_UNUSED(work);
    bool showing_time_sync = false;

    if (s_uptime_label) {
        char buf[24];

        showing_time_sync = format_time_sync(buf, sizeof(buf));
        if (!showing_time_sync) {
            format_uptime(buf, sizeof(buf));
        }

        apply_display_mode(showing_time_sync);
        lv_label_set_text(s_uptime_label, buf);
    }

    k_work_schedule_for_queue(zmk_display_work_q(), &uptime_work,
                              showing_time_sync && time_sync_wants_seconds()
                                  ? TIME_SYNC_PERIOD
                                  : UPTIME_PERIOD);
}

int zmk_widget_uptime_info_init(struct zmk_widget_uptime_info *widget, lv_obj_t *parent) {
    s_uptime_label = lv_label_create(parent);
    widget->obj = s_uptime_label;

    lv_label_set_text(s_uptime_label, "UP 0:00");
    lv_obj_set_style_text_align(s_uptime_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    apply_display_mode(false);

    k_work_init_delayable(&uptime_work, uptime_update_cb);
    k_work_schedule_for_queue(zmk_display_work_q(), &uptime_work, K_NO_WAIT);

    return 0;
}

lv_obj_t *zmk_widget_uptime_info_obj(struct zmk_widget_uptime_info *widget) {
    return widget->obj;
}

void ring_uptime_info_apply_theme(void) {
    if (s_uptime_label) {
        apply_display_mode(s_showing_time_sync);
    }
}

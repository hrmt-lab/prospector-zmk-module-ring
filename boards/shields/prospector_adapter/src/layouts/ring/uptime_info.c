#include "uptime_info.h"

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zmk/display.h>

#include "ring_theme.h"

#define UPTIME_X      198
#define UPTIME_Y      10
#define UPTIME_W      64
#define UPTIME_PERIOD K_MINUTES(1)

static lv_obj_t *s_uptime_label = NULL;
static struct k_work_delayable uptime_work;

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

static void uptime_update_cb(struct k_work *work) {
    ARG_UNUSED(work);

    if (s_uptime_label) {
        char buf[16];
        format_uptime(buf, sizeof(buf));
        lv_label_set_text(s_uptime_label, buf);
    }

    k_work_schedule_for_queue(zmk_display_work_q(), &uptime_work, UPTIME_PERIOD);
}

int zmk_widget_uptime_info_init(struct zmk_widget_uptime_info *widget, lv_obj_t *parent) {
    s_uptime_label = lv_label_create(parent);
    widget->obj = s_uptime_label;

    lv_label_set_text(s_uptime_label, "UP 0:00");
    lv_obj_set_style_text_font(s_uptime_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_uptime_label, lv_color_hex(ring_color_text_off()), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_uptime_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_width(s_uptime_label, UPTIME_W);
    lv_obj_set_pos(s_uptime_label, UPTIME_X, UPTIME_Y);

    k_work_init_delayable(&uptime_work, uptime_update_cb);
    k_work_schedule_for_queue(zmk_display_work_q(), &uptime_work, K_NO_WAIT);

    return 0;
}

lv_obj_t *zmk_widget_uptime_info_obj(struct zmk_widget_uptime_info *widget) {
    return widget->obj;
}

void ring_uptime_info_apply_theme(void) {
    if (s_uptime_label) {
        lv_obj_set_style_text_color(s_uptime_label,
            lv_color_hex(ring_color_text_off()), LV_PART_MAIN);
    }
}

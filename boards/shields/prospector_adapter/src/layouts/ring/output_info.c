#include "output_info.h"

#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/ble.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

#include "display_colors.h"

extern lv_font_t DINishCondensed_SemiBold_20;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static uint8_t active_profile_index  = 0;
static enum zmk_transport active_transport = ZMK_TRANSPORT_USB;
static uint32_t keys_count = 0;

// ──────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────

static void format_keys_count(char *buf, size_t len, uint32_t count) {
    if (count < 1000) {
        snprintf(buf, len, "%u", count);
    } else if (count < 100000) {
        snprintf(buf, len, "%u,%03u", count / 1000, count % 1000);
    } else {
        snprintf(buf, len, "99k+");
    }
}

static void update_out_display(void) {
    struct zmk_widget_output_info *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        bool is_ble = (active_transport == ZMK_TRANSPORT_BLE);

        lv_label_set_text(widget->out_proto, is_ble ? "BLE" : "USB");
        lv_obj_set_style_text_color(
            widget->out_proto,
            lv_color_hex(is_ble ? RING_COLOR_TEXT_SEC : RING_COLOR_ACCENT),
            LV_PART_MAIN);

        if (is_ble) {
            char profile_buf[4];
            snprintf(profile_buf, sizeof(profile_buf), "%d", active_profile_index + 1);
            lv_label_set_text(widget->out_profile, profile_buf);
            lv_obj_remove_flag(widget->out_profile, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(widget->dongle_group, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(widget->out_profile, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(widget->dongle_group, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ──────────────────────────────────────────────────────────
// Endpoint / profile listeners (ZMK_LISTENER - follows radii pattern)
// ──────────────────────────────────────────────────────────

static int endpoint_changed_listener(const zmk_event_t *eh) {
    const struct zmk_endpoint_changed *ev = as_zmk_endpoint_changed(eh);
    if (ev) {
        struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();
        active_transport = selected.transport;
        update_out_display();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int profile_changed_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);
    if (ev) {
        active_profile_index = ev->index;
        update_out_display();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_output_info_endpoint, endpoint_changed_listener);
ZMK_SUBSCRIPTION(widget_output_info_endpoint, zmk_endpoint_changed);

ZMK_LISTENER(widget_output_info_profile, profile_changed_listener);
ZMK_SUBSCRIPTION(widget_output_info_profile, zmk_ble_active_profile_changed);

// ──────────────────────────────────────────────────────────
// Dongle battery (DISPLAY_WIDGET_LISTENER for thread-safe LVGL access)
// ──────────────────────────────────────────────────────────

struct output_info_dongle_state {
    uint8_t level;
};

static void dongle_batt_update_cb(struct output_info_dongle_state state) {
    struct zmk_widget_output_info *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        lv_label_set_text_fmt(widget->dongle_value, "%d", state.level);
    }
}

static struct output_info_dongle_state dongle_batt_get_state(const zmk_event_t *eh) {
    if (eh == NULL) {
        return (struct output_info_dongle_state){.level = 0};
    }
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return (struct output_info_dongle_state){.level = 0};
    }
    return (struct output_info_dongle_state){.level = ev->state_of_charge};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_info_dongle, struct output_info_dongle_state,
                            dongle_batt_update_cb, dongle_batt_get_state);
ZMK_SUBSCRIPTION(widget_output_info_dongle, zmk_battery_state_changed);

// ──────────────────────────────────────────────────────────
// Keys counter (ZMK_LISTENER - follows radii output pattern)
// ──────────────────────────────────────────────────────────

static int keys_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *kc = as_zmk_keycode_state_changed(eh);
    if (kc && kc->state && kc->usage_page == HID_USAGE_KEY) {
        // Skip HID modifier keycodes 0xE0-0xE7
        if (kc->keycode < 0xE0 || kc->keycode > 0xE7) {
            keys_count++;
            char buf[8];
            format_keys_count(buf, sizeof(buf), keys_count);
            struct zmk_widget_output_info *widget;
            SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
                lv_label_set_text(widget->keys_value, buf);
            }
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(widget_output_info_keys, keys_listener);
ZMK_SUBSCRIPTION(widget_output_info_keys, zmk_keycode_state_changed);

// ──────────────────────────────────────────────────────────
// Widget init / obj
// ──────────────────────────────────────────────────────────

int zmk_widget_output_info_init(struct zmk_widget_output_info *widget, lv_obj_t *parent) {
    widget->obj = parent;

    // ── Dongle battery group (header area, hidden until BLE) ──
    widget->dongle_group = lv_obj_create(parent);
    lv_obj_set_size(widget->dongle_group, 60, 20);
    lv_obj_set_pos(widget->dongle_group, 206, 8);
    lv_obj_set_style_bg_opa(widget->dongle_group, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->dongle_group, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->dongle_group, 0, LV_PART_MAIN);

    lv_obj_t *dongle_label = lv_label_create(widget->dongle_group);
    lv_label_set_text(dongle_label, "D");
    lv_obj_set_style_text_font(dongle_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(dongle_label, lv_color_hex(RING_COLOR_TEXT_TER), LV_PART_MAIN);
    lv_obj_set_pos(dongle_label, 0, 0);

    widget->dongle_value = lv_label_create(widget->dongle_group);
    lv_label_set_text(widget->dongle_value, "---");
    lv_obj_set_style_text_font(widget->dongle_value, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->dongle_value, lv_color_hex(RING_COLOR_TEXT_SEC), LV_PART_MAIN);
    lv_obj_set_pos(widget->dongle_value, 14, 0);

    lv_obj_add_flag(widget->dongle_group, LV_OBJ_FLAG_HIDDEN);

    // ── OUT section ──────────────────────────────────────────
    lv_obj_t *out_header = lv_label_create(parent);
    lv_label_set_text(out_header, "OUT");
    lv_obj_set_style_text_font(out_header, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(out_header, lv_color_hex(RING_COLOR_TEXT_TER), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(out_header, 2, LV_PART_MAIN);
    lv_obj_set_pos(out_header, 194, 172);

    widget->out_proto = lv_label_create(parent);
    lv_label_set_text(widget->out_proto, "USB");
    lv_obj_set_style_text_font(widget->out_proto, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->out_proto, lv_color_hex(RING_COLOR_ACCENT), LV_PART_MAIN);
    lv_obj_set_pos(widget->out_proto, 228, 172);

    widget->out_profile = lv_label_create(parent);
    lv_label_set_text(widget->out_profile, "1");
    lv_obj_set_style_text_font(widget->out_profile, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->out_profile, lv_color_hex(RING_COLOR_ACCENT), LV_PART_MAIN);
    lv_obj_set_pos(widget->out_profile, 252, 172);
    lv_obj_add_flag(widget->out_profile, LV_OBJ_FLAG_HIDDEN);

    // ── KEYS section ─────────────────────────────────────────
    lv_obj_t *keys_header = lv_label_create(parent);
    lv_label_set_text(keys_header, "KEYS");
    lv_obj_set_style_text_font(keys_header, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(keys_header, lv_color_hex(RING_COLOR_TEXT_TER), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(keys_header, 2, LV_PART_MAIN);
    lv_obj_set_pos(keys_header, 194, 200);

    widget->keys_value = lv_label_create(parent);
    lv_label_set_text(widget->keys_value, "0");
    lv_obj_set_style_text_font(widget->keys_value, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->keys_value, lv_color_hex(RING_COLOR_TEXT_PRI), LV_PART_MAIN);
    lv_obj_set_pos(widget->keys_value, 228, 200);

    // Initialise endpoint/profile state from current values
    if (sys_slist_is_empty(&widgets)) {
        active_profile_index = zmk_ble_active_profile_index();
        struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();
        active_transport = selected.transport;
    }

    sys_slist_append(&widgets, &widget->node);

    widget_output_info_dongle_init();

    // Apply initial OUT display state
    update_out_display();

    return 0;
}

lv_obj_t *zmk_widget_output_info_obj(struct zmk_widget_output_info *widget) {
    return widget->obj;
}

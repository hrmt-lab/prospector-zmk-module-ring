#include "keys_info.h"

#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <stdio.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

#include "display_colors.h"
#include "ring_theme.h"

extern lv_font_t DINishCondensed_SemiBold_20;

#define SIDE_PANEL_X    206
#define SIDE_PANEL_W    64
#define LAST_HEADER_Y   146
#define LAST_VALUE_Y    160
#define KEYS_HEADER_Y   184
#define KEYS_VALUE_Y    200

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static uint32_t keys_count = 0;
#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
static char last_key_name[8] = "---";
static lv_obj_t *s_last_header = NULL;
#endif
static lv_obj_t *s_keys_header = NULL;

#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
static const char *keycode_name(uint32_t keycode) {
    static const char *letters[] = {
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
    };
    static const char *numbers[] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"
    };

    if (keycode >= HID_USAGE_KEY_KEYBOARD_A && keycode <= HID_USAGE_KEY_KEYBOARD_Z) {
        return letters[keycode - HID_USAGE_KEY_KEYBOARD_A];
    }
    if (keycode >= HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION &&
        keycode <= HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS) {
        return numbers[keycode - HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION];
    }
    if (keycode >= HID_USAGE_KEY_KEYBOARD_F1 && keycode <= HID_USAGE_KEY_KEYBOARD_F12) {
        static char f_key[4];
        snprintf(f_key, sizeof(f_key), "F%u",
                 (unsigned int)(keycode - HID_USAGE_KEY_KEYBOARD_F1 + 1));
        return f_key;
    }

    switch (keycode) {
    case HID_USAGE_KEY_KEYBOARD_RETURN_ENTER: return "ENTER";
    case HID_USAGE_KEY_KEYBOARD_ESCAPE: return "ESC";
    case HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE: return "BSPC";
    case HID_USAGE_KEY_KEYBOARD_TAB: return "TAB";
    case HID_USAGE_KEY_KEYBOARD_SPACEBAR: return "SPACE";
    case HID_USAGE_KEY_KEYBOARD_MINUS_AND_UNDERSCORE: return "-";
    case HID_USAGE_KEY_KEYBOARD_EQUAL_AND_PLUS: return "=";
    case HID_USAGE_KEY_KEYBOARD_LEFT_BRACKET_AND_LEFT_BRACE: return "[";
    case HID_USAGE_KEY_KEYBOARD_RIGHT_BRACKET_AND_RIGHT_BRACE: return "]";
    case HID_USAGE_KEY_KEYBOARD_BACKSLASH_AND_PIPE: return "\\";
    case HID_USAGE_KEY_KEYBOARD_SEMICOLON_AND_COLON: return ";";
    case HID_USAGE_KEY_KEYBOARD_APOSTROPHE_AND_QUOTE: return "'";
    case HID_USAGE_KEY_KEYBOARD_GRAVE_ACCENT_AND_TILDE: return "`";
    case HID_USAGE_KEY_KEYBOARD_COMMA_AND_LESS_THAN: return ",";
    case HID_USAGE_KEY_KEYBOARD_PERIOD_AND_GREATER_THAN: return ".";
    case HID_USAGE_KEY_KEYBOARD_SLASH_AND_QUESTION_MARK: return "/";
    case HID_USAGE_KEY_KEYBOARD_CAPS_LOCK: return "CAPS";
    case HID_USAGE_KEY_KEYBOARD_PRINTSCREEN: return "PSCR";
    case HID_USAGE_KEY_KEYBOARD_SCROLL_LOCK: return "SLCK";
    case HID_USAGE_KEY_KEYBOARD_PAUSE: return "PAUS";
    case HID_USAGE_KEY_KEYBOARD_INSERT: return "INS";
    case HID_USAGE_KEY_KEYBOARD_HOME: return "HOME";
    case HID_USAGE_KEY_KEYBOARD_PAGEUP: return "PGUP";
    case HID_USAGE_KEY_KEYBOARD_DELETE_FORWARD: return "DEL";
    case HID_USAGE_KEY_KEYBOARD_END: return "END";
    case HID_USAGE_KEY_KEYBOARD_PAGEDOWN: return "PGDN";
    case HID_USAGE_KEY_KEYBOARD_RIGHTARROW: return "RGT";
    case HID_USAGE_KEY_KEYBOARD_LEFTARROW: return "LFT";
    case HID_USAGE_KEY_KEYBOARD_DOWNARROW: return "DOWN";
    case HID_USAGE_KEY_KEYBOARD_UPARROW: return "UP";
    case HID_USAGE_KEY_KEYPAD_ENTER: return "KENT";
    case HID_USAGE_KEY_KEYBOARD_INTERNATIONAL4: return "INT4";
    case HID_USAGE_KEY_KEYBOARD_INTERNATIONAL5: return "INT5";
    case HID_USAGE_KEY_KEYBOARD_LEFTCONTROL: return "LCTL";
    case HID_USAGE_KEY_KEYBOARD_LEFTSHIFT: return "LSFT";
    case HID_USAGE_KEY_KEYBOARD_LEFTALT: return "LALT";
    case HID_USAGE_KEY_KEYBOARD_LEFT_GUI: return "LGUI";
    case HID_USAGE_KEY_KEYBOARD_RIGHTCONTROL: return "RCTL";
    case HID_USAGE_KEY_KEYBOARD_RIGHTSHIFT: return "RSFT";
    case HID_USAGE_KEY_KEYBOARD_RIGHTALT: return "RALT";
    case HID_USAGE_KEY_KEYBOARD_RIGHT_GUI: return "RGUI";
    default: return NULL;
    }
}
#endif

static void format_keys_count(char *buf, size_t len, uint32_t count) {
    if (count < 1000) {
        snprintf(buf, len, "%u", count);
    } else if (count < 100000) {
        snprintf(buf, len, "%u,%03u", count / 1000, count % 1000);
    } else {
        snprintf(buf, len, "99k+");
    }
}

struct keys_info_state {
    uint32_t count;
#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
    char last[8];
#endif
};

static void keys_info_update_cb(struct keys_info_state state) {
    char buf[8];
    format_keys_count(buf, sizeof(buf), state.count);

    struct zmk_widget_keys_info *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
        lv_label_set_text(widget->last_value, state.last);
#endif
        lv_label_set_text(widget->keys_value, buf);
    }
}

static struct keys_info_state keys_info_get_state(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *kc =
        eh ? as_zmk_keycode_state_changed(eh) : NULL;

    if (kc && kc->state && kc->usage_page == HID_USAGE_KEY) {
#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
        const char *name = keycode_name(kc->keycode);
        if (name) {
            snprintf(last_key_name, sizeof(last_key_name), "%s", name);
        } else {
            snprintf(last_key_name, sizeof(last_key_name), "0x%02X", kc->keycode & 0xFF);
        }
#endif

        /* Skip HID modifier keycodes 0xE0-0xE7. */
        if (kc->keycode < 0xE0 || kc->keycode > 0xE7) {
            keys_count++;
        }
    }

    return (struct keys_info_state){
        .count = keys_count,
#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
        .last = {last_key_name[0], last_key_name[1], last_key_name[2], last_key_name[3],
                 last_key_name[4], last_key_name[5], last_key_name[6], last_key_name[7]},
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_keys_info, struct keys_info_state,
                            keys_info_update_cb, keys_info_get_state)
ZMK_SUBSCRIPTION(widget_keys_info, zmk_keycode_state_changed);

int zmk_widget_keys_info_init(struct zmk_widget_keys_info *widget, lv_obj_t *parent) {
    widget->obj = parent;

#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
    s_last_header = lv_label_create(parent);
    lv_label_set_text(s_last_header, "LAST");
    lv_obj_set_style_text_font(s_last_header, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_last_header, lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(s_last_header, 2, LV_PART_MAIN);
    lv_obj_set_pos(s_last_header, SIDE_PANEL_X, LAST_HEADER_Y);

    widget->last_value = lv_label_create(parent);
    lv_label_set_text(widget->last_value, last_key_name);
    lv_obj_set_style_text_font(widget->last_value, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->last_value, lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    lv_obj_set_pos(widget->last_value, SIDE_PANEL_X, LAST_VALUE_Y);
    lv_obj_set_width(widget->last_value, SIDE_PANEL_W);
    lv_obj_set_style_text_align(widget->last_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
#endif

    s_keys_header = lv_label_create(parent);
    lv_label_set_text(s_keys_header, "KEYS");
    lv_obj_set_style_text_font(s_keys_header, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_keys_header, lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(s_keys_header, 2, LV_PART_MAIN);
    lv_obj_set_pos(s_keys_header, SIDE_PANEL_X, KEYS_HEADER_Y);

    widget->keys_value = lv_label_create(parent);
    lv_label_set_text(widget->keys_value, "0");
    lv_obj_set_style_text_font(widget->keys_value, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(widget->keys_value, lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    lv_obj_set_pos(widget->keys_value, SIDE_PANEL_X, KEYS_VALUE_Y);
    lv_obj_set_width(widget->keys_value, SIDE_PANEL_W);
    lv_obj_set_style_text_align(widget->keys_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    sys_slist_append(&widgets, &widget->node);
    widget_keys_info_init();

    return 0;
}

lv_obj_t *zmk_widget_keys_info_obj(struct zmk_widget_keys_info *widget) {
    return widget->obj;
}

void ring_keys_info_apply_theme(void) {
#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
    if (s_last_header) {
        lv_obj_set_style_text_color(s_last_header,
            lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    }
#endif
    if (s_keys_header) {
        lv_obj_set_style_text_color(s_keys_header,
            lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    }

    struct zmk_widget_keys_info *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
#if IS_ENABLED(CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY)
        lv_obj_set_style_text_color(widget->last_value,
            lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
#endif
        lv_obj_set_style_text_color(widget->keys_value,
            lv_color_hex(ring_color_text_pri()), LV_PART_MAIN);
    }
}

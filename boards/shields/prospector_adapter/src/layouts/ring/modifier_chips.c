#include "modifier_chips.h"

#include <zmk/display.h>
#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
#include <zmk/events/caps_word_state_changed.h>
#endif
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

#include "display_colors.h"

extern lv_font_t DINishCondensed_SemiBold_20;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

#define CHIP_W 28
#define CHIP_H 22
#define CHIP_RADIUS 4

static const int16_t MOD_CHIP_X[4]    = {206, 238, 206, 238};
static const int16_t MOD_CHIP_Y[4]    = {64,  64,  90,  90};
static const char   *MOD_CHIP_TEXT[4] = {"CTRL", "SHFT", "ALT", "GUI"};

static const int16_t STATE_CHIP_X[2]    = {206, 238};
static const int16_t STATE_CHIP_Y[2]    = {138, 138};
static const char   *STATE_CHIP_TEXT[2] = {"CAPS", "IME"};

#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
static bool caps_word_active = false;
#endif
static bool ime_active = false;

struct modifier_chips_state {
    bool mods[4];
#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
    bool caps_word;
#endif
    bool ime_active;
};

static void set_chip_active(lv_obj_t *chip, bool active) {
    lv_obj_t *label = lv_obj_get_child(chip, 0);
    if (active) {
        lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(chip, lv_color_hex(RING_COLOR_ACCENT), LV_PART_MAIN);
        lv_obj_set_style_border_width(chip, 0, LV_PART_MAIN);
        if (label) {
            lv_obj_set_style_text_color(label, lv_color_hex(RING_COLOR_BG), LV_PART_MAIN);
        }
    } else {
        lv_obj_set_style_bg_opa(chip, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_color(chip, lv_color_hex(RING_COLOR_TRACK), LV_PART_MAIN);
        lv_obj_set_style_border_width(chip, 1, LV_PART_MAIN);
        if (label) {
            lv_obj_set_style_text_color(label, lv_color_hex(RING_COLOR_TEXT_TER), LV_PART_MAIN);
        }
    }
}

static void modifier_chips_update_cb(struct modifier_chips_state state) {
    struct zmk_widget_modifier_chips *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_chip_active(widget->mod_chips[0], state.mods[0]);
        set_chip_active(widget->mod_chips[1], state.mods[1]);
        set_chip_active(widget->mod_chips[2], state.mods[2]);
        set_chip_active(widget->mod_chips[3], state.mods[3]);
#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
        set_chip_active(widget->state_chips[0], state.caps_word);
#else
        set_chip_active(widget->state_chips[0], false);
#endif
        set_chip_active(widget->state_chips[1], state.ime_active);
    }
}

static struct modifier_chips_state modifier_chips_get_state(const zmk_event_t *eh) {
    zmk_mod_flags_t mods = zmk_hid_get_explicit_mods();

#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
    if (eh != NULL) {
        const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
        if (ev != NULL) {
            caps_word_active = ev->active;
        }
    }
#endif

    if (eh != NULL) {
        const struct zmk_keycode_state_changed *kc_ev = as_zmk_keycode_state_changed(eh);
        if (kc_ev != NULL && kc_ev->state) {
            if (kc_ev->usage_page == HID_USAGE_KEY &&
                kc_ev->keycode == HID_USAGE_KEY_KEYBOARD_INTERNATIONAL4) {
                ime_active = true;
            } else if (kc_ev->usage_page == HID_USAGE_KEY &&
                       kc_ev->keycode == HID_USAGE_KEY_KEYBOARD_INTERNATIONAL5) {
                ime_active = false;
            }
        }
    }

    return (struct modifier_chips_state){
        .mods = {
            (mods & (MOD_LCTL | MOD_RCTL)) != 0,
            (mods & (MOD_LSFT | MOD_RSFT)) != 0,
            (mods & (MOD_LALT | MOD_RALT)) != 0,
            (mods & (MOD_LGUI | MOD_RGUI)) != 0,
        },
#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
        .caps_word  = caps_word_active,
#endif
        .ime_active = ime_active,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_modifier_chips, struct modifier_chips_state,
                            modifier_chips_update_cb, modifier_chips_get_state)
ZMK_SUBSCRIPTION(widget_modifier_chips, zmk_keycode_state_changed);
#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
ZMK_SUBSCRIPTION(widget_modifier_chips, zmk_caps_word_state_changed);
#endif

static lv_obj_t *create_chip(lv_obj_t *parent, int16_t x, int16_t y, const char *text) {
    lv_obj_t *chip = lv_obj_create(parent);
    lv_obj_set_size(chip, CHIP_W, CHIP_H);
    lv_obj_set_pos(chip, x, y);
    lv_obj_set_style_radius(chip, CHIP_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chip, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(chip, lv_color_hex(RING_COLOR_TRACK), LV_PART_MAIN);
    lv_obj_set_style_border_width(chip, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chip, 0, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(chip);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(RING_COLOR_TEXT_TER), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    return chip;
}

static lv_obj_t *create_section_label(lv_obj_t *parent, int16_t x, int16_t y, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(RING_COLOR_TEXT_TER), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(label, 1, LV_PART_MAIN);
    lv_obj_set_pos(label, x, y);
    return label;
}

int zmk_widget_modifier_chips_init(struct zmk_widget_modifier_chips *widget, lv_obj_t *parent) {
    widget->obj = parent;

    create_section_label(parent, 206, 44, "MOD");

    for (int i = 0; i < 4; i++) {
        widget->mod_chips[i] = create_chip(parent, MOD_CHIP_X[i], MOD_CHIP_Y[i], MOD_CHIP_TEXT[i]);
    }

    create_section_label(parent, 206, 118, "STATE");

    for (int i = 0; i < 2; i++) {
        widget->state_chips[i] =
            create_chip(parent, STATE_CHIP_X[i], STATE_CHIP_Y[i], STATE_CHIP_TEXT[i]);
    }

    sys_slist_append(&widgets, &widget->node);
    widget_modifier_chips_init();

    return 0;
}

lv_obj_t *zmk_widget_modifier_chips_obj(struct zmk_widget_modifier_chips *widget) {
    return widget->obj;
}

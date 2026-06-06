#include "ai_usage.h"

#ifdef CONFIG_PROSPECTOR_RING_AI_USAGE

#include <stdio.h>
#include <string.h>

#include <lvgl.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>

#include "battery_rings.h"
#include "brightness_info.h"
#include "keys_info.h"
#include "modifier_chips.h"
#include "ring_theme.h"

#if IS_ENABLED(CONFIG_RAWHID_APP_AI_USAGE)
#include <rawhid_app/ai_usage.h>
#endif

#if IS_ENABLED(CONFIG_PROSPECTOR_RING_AI_USAGE_TOGGLE_KEY)
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#endif

/* status_screen.c has no header of its own. */
void ring_status_screen_set_main_decor_visible(bool visible);

/* ── Provider brand colors (spec §2/§3.2, fixed regardless of theme) ──── */
#define AI_COLOR_CLAUDE 0xCC7E5Au
#define AI_COLOR_CODEX  0x7B6FE8u

/* ── AI usage flag bits (RawHID Host AI Usage spec v1) ───────────────── */
#define AI_FLAG_5H_VALID  BIT(0)
#define AI_FLAG_7D_VALID  BIT(1)
#define AI_FLAG_ESTIMATED BIT(2)
#define AI_FLAG_QUOTA     BIT(4)
#define AI_FLAG_STALE     BIT(5)
#define AI_FLAG_ERROR     BIT(7)

/* Provider ids (match hitsuki46 spec). */
#define AI_PROVIDER_CODEX  1
#define AI_PROVIDER_CLAUDE 2

/* ── Bar geometry ────────────────────────────────────────────────────
 * Top section stacks (no overlap) using real font line heights:
 *   kb name (~y11-29) / layer name cormorant_30 32px (y29-61)
 *   AI USAGE label (y62-77) / 5H,7D column labels (y78-93) / bars (y94+).  */
#define BAR_W        52
#define BAR_TOP      94
#define BAR_MAX_H    84
#define BAR_RADIUS   5
#define AI_LABEL_Y   62
#define COL_LABEL_Y  78
#define VALUE_Y      180
#define RESET_Y      200
#define TOOL_Y       218
#define REFRESH_PERIOD K_SECONDS(2)

/* Bar index: 0=Claude 5H, 1=Claude 7D, 2=Codex 5H, 3=Codex 7D. */
static const uint8_t  BAR_PROVIDER[4] = {AI_PROVIDER_CLAUDE, AI_PROVIDER_CLAUDE,
                                         AI_PROVIDER_CODEX, AI_PROVIDER_CODEX};
static const bool     BAR_IS_5H[4]    = {true, false, true, false};
/* Bar group centered on the 280px display: span 15..265 (width 250), center 140. */
static const int16_t  BAR_X[4]        = {15, 81, 147, 213};
static const int16_t  BAR_CX[4]       = {41, 107, 173, 239};
static const uint32_t BAR_COLOR[4]    = {AI_COLOR_CLAUDE, AI_COLOR_CLAUDE,
                                         AI_COLOR_CODEX, AI_COLOR_CODEX};

/* ── State ───────────────────────────────────────────────────────────── */
static lv_obj_t *s_root      = NULL;
static lv_obj_t *s_bar_bg[4]   = {0};
static lv_obj_t *s_bar_fill[4] = {0};
static lv_obj_t *s_value[4]    = {0};
static lv_obj_t *s_reset[4]    = {0};
static lv_obj_t *s_col_label[4] = {0};
static lv_obj_t *s_tool[2]     = {0};
static lv_obj_t *s_ai_label  = NULL;
static lv_obj_t *s_hdiv      = NULL;
static lv_obj_t *s_vdiv      = NULL;
static bool      s_shown     = false;

static struct k_work_delayable refresh_work;

/* ── Neutral data view, decoupled from the hitsuki46 header ───────────── */
struct ai_view {
    bool present;
    uint8_t flags;
    uint16_t five_bp;
    uint16_t seven_bp;
    uint32_t five_reset;
    uint32_t seven_reset;
    uint32_t updated_unix;
    int64_t received_uptime_ms;
};

static struct ai_view fetch_provider(uint8_t provider) {
    struct ai_view v = {0};
#if IS_ENABLED(CONFIG_RAWHID_APP_AI_USAGE)
    struct rawhid_app_ai_usage_provider p;
    if (rawhid_app_ai_usage_get(provider, &p)) {
        v.present = true;
        v.flags = p.flags;
        v.five_bp = p.five_hour_used_bp;
        v.seven_bp = p.seven_day_used_bp;
        v.five_reset = p.five_hour_reset_unix;
        v.seven_reset = p.seven_day_reset_unix;
        v.updated_unix = p.updated_unix;
        v.received_uptime_ms = p.received_uptime_ms;
    }
#else
    ARG_UNUSED(provider);
#endif
    return v;
}

/* ── Bar update ──────────────────────────────────────────────────────── */

static void update_bar(int idx, const struct ai_view *v) {
    bool is_5h = BAR_IS_5H[idx];
    uint16_t bp = is_5h ? v->five_bp : v->seven_bp;
    uint8_t valid_bit = is_5h ? AI_FLAG_5H_VALID : AI_FLAG_7D_VALID;

    char buf[8];
    int pct = -1; /* -1 = no fill */

    if (!v->present) {
        strcpy(buf, "--");
    } else if (!(v->flags & valid_bit)) {
        /* Invalid window: ERR only when an error is present, otherwise no data. */
        strcpy(buf, (v->flags & AI_FLAG_ERROR) ? "ERR" : "--");
    } else {
        pct = bp / 100;
        if (pct > 100) {
            pct = 100;
        }
        /* valid window: stale '*' takes priority over estimated 'e'. */
        const char *sfx = (v->flags & AI_FLAG_STALE)       ? "*"
                          : (v->flags & AI_FLAG_ESTIMATED) ? "e"
                                                           : "";
        snprintf(buf, sizeof(buf), "%d%%%s", pct, sfx);
    }

    if (s_value[idx]) {
        lv_label_set_text(s_value[idx], buf);
    }

    if (!s_bar_fill[idx]) {
        return;
    }

    if (pct <= 0) {
        lv_obj_add_flag(s_bar_fill[idx], LV_OBJ_FLAG_HIDDEN);
    } else {
        /* Scale 0..100% across the bar height; keep >=1px so tiny usage shows. */
        int h = pct * BAR_MAX_H / 100;
        if (h < 1) {
            h = 1;
        } else if (h > BAR_MAX_H) {
            h = BAR_MAX_H;
        }
        lv_obj_clear_flag(s_bar_fill[idx], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_height(s_bar_fill[idx], h);
        lv_obj_set_y(s_bar_fill[idx], BAR_TOP + (BAR_MAX_H - h));
    }
}

/* Relative countdown until the window resets. Shown only for quota-source data
 * with a valid window and a non-zero reset time; blank otherwise. */
static void update_reset(int idx, const struct ai_view *v) {
    if (!s_reset[idx]) {
        return;
    }

    bool is_5h = BAR_IS_5H[idx];
    uint8_t valid_bit = is_5h ? AI_FLAG_5H_VALID : AI_FLAG_7D_VALID;
    uint32_t reset = is_5h ? v->five_reset : v->seven_reset;

    if (!v->present || !(v->flags & valid_bit) || !(v->flags & AI_FLAG_QUOTA) || reset == 0) {
        lv_label_set_text(s_reset[idx], "");
        return;
    }

    int64_t elapsed = (k_uptime_get() - v->received_uptime_ms) / 1000;
    int64_t remaining = (int64_t)reset - (int64_t)v->updated_unix - elapsed;
    if (remaining < 0) {
        remaining = 0;
    }

    char buf[12];
    if (remaining >= 86400) {
        /* >= 1 day: days + hours. */
        snprintf(buf, sizeof(buf), "%dd %dh", (int)(remaining / 86400),
                 (int)((remaining % 86400) / 3600));
    } else if (remaining >= 3600) {
        /* < 1 day: hours + minutes (no day field). */
        snprintf(buf, sizeof(buf), "%dh %dm", (int)(remaining / 3600),
                 (int)((remaining % 3600) / 60));
    } else {
        /* < 1 hour: minutes only. */
        snprintf(buf, sizeof(buf), "%dm", (int)(remaining / 60));
    }
    lv_label_set_text(s_reset[idx], buf);
}

static void update_bars(void) {
    struct ai_view claude = fetch_provider(AI_PROVIDER_CLAUDE);
    struct ai_view codex = fetch_provider(AI_PROVIDER_CODEX);

    for (int i = 0; i < 4; i++) {
        const struct ai_view *v = (BAR_PROVIDER[i] == AI_PROVIDER_CLAUDE) ? &claude : &codex;
        update_bar(i, v);
        update_reset(i, v);
    }
}

static void refresh_work_cb(struct k_work *work) {
    ARG_UNUSED(work);
    if (!s_shown) {
        return;
    }
    update_bars();
    k_work_schedule_for_queue(zmk_display_work_q(), &refresh_work, REFRESH_PERIOD);
}

/* ── Build helpers ───────────────────────────────────────────────────── */

static lv_obj_t *make_line(lv_obj_t *parent, int16_t x, int16_t y, int16_t w, int16_t h) {
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_size(line, w, h);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_style_bg_color(line, lv_color_hex(ring_color_track()), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
    return line;
}

static lv_obj_t *make_label(lv_obj_t *parent, int16_t cx, int16_t y, int16_t w,
                            const lv_font_t *font, uint32_t color, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_width(label, w);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(label, cx - w / 2, y);
    return label;
}

static void build_root(lv_obj_t *root) {
    /* Group divider between the two providers (no horizontal header rule:
     * the top section is tight and a horizontal line would crowd it). */
    s_hdiv = NULL;
    s_vdiv = make_line(root, 140, COL_LABEL_Y, 1, (TOOL_Y - COL_LABEL_Y));

    s_ai_label = lv_label_create(root);
    lv_label_set_text(s_ai_label, "AI USAGE");
    lv_obj_set_style_text_font(s_ai_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ai_label, lv_color_hex(ring_color_text_sec()), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(s_ai_label, 2, LV_PART_MAIN);
    lv_obj_set_pos(s_ai_label, 14, AI_LABEL_Y);

    for (int i = 0; i < 4; i++) {
        /* Column label (5H / 7D). */
        s_col_label[i] = make_label(root, BAR_CX[i], COL_LABEL_Y, 40, &lv_font_montserrat_12,
                                    ring_color_text_sec(), BAR_IS_5H[i] ? "5H" : "7D");

        /* Bar background (track). */
        s_bar_bg[i] = lv_obj_create(root);
        lv_obj_remove_style_all(s_bar_bg[i]);
        lv_obj_set_size(s_bar_bg[i], BAR_W, BAR_MAX_H);
        lv_obj_set_pos(s_bar_bg[i], BAR_X[i], BAR_TOP);
        lv_obj_set_style_radius(s_bar_bg[i], BAR_RADIUS, LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_bar_bg[i], lv_color_hex(ring_color_track()), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(s_bar_bg[i], LV_OPA_COVER, LV_PART_MAIN);

        /* Bar fill (brand color, height set on update). */
        s_bar_fill[i] = lv_obj_create(root);
        lv_obj_remove_style_all(s_bar_fill[i]);
        lv_obj_set_size(s_bar_fill[i], BAR_W, 0);
        lv_obj_set_pos(s_bar_fill[i], BAR_X[i], BAR_TOP + BAR_MAX_H);
        lv_obj_set_style_radius(s_bar_fill[i], BAR_RADIUS, LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_bar_fill[i], lv_color_hex(BAR_COLOR[i]), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(s_bar_fill[i], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_add_flag(s_bar_fill[i], LV_OBJ_FLAG_HIDDEN);

        /* Usage value (brand color). */
        s_value[i] = make_label(root, BAR_CX[i], VALUE_Y, BAR_W, &lv_font_montserrat_14,
                                BAR_COLOR[i], "--");

        /* Reset countdown (dim, filled on update). */
        s_reset[i] = make_label(root, BAR_CX[i], RESET_Y, BAR_W, &lv_font_montserrat_12,
                                ring_color_text_sec(), "");
    }

    /* Tool names, centered under each provider's two bars:
     * Claude = midpoint of bars 0/1 (cx 41,107 -> 74);
     * Codex  = midpoint of bars 2/3 (cx 173,239 -> 206). */
    s_tool[0] = make_label(root, 74, TOOL_Y, 80, &lv_font_montserrat_12, AI_COLOR_CLAUDE, "Claude");
    s_tool[1] = make_label(root, 206, TOOL_Y, 80, &lv_font_montserrat_12, AI_COLOR_CODEX, "Codex");
}

/* ── Show / hide ─────────────────────────────────────────────────────── */

void ring_show_ai_usage(void) {
    if (s_root != NULL) {
        return;
    }

    /* Move shared objects to the AI Usage layout / hide Main-only objects. */
    ring_battery_rings_apply_layout(true);
    ring_modifier_chips_apply_layout(true);
    ring_keys_info_set_visible(false);
    ring_brightness_info_set_visible(false);
    ring_status_screen_set_main_decor_visible(false);

    s_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(s_root);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_size(s_root, 280, 240);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    build_root(s_root);

    /* Keep the shared (sibling) objects above the AI Usage container. */
    lv_obj_move_background(s_root);

    s_shown = true;
    update_bars();
    k_work_schedule_for_queue(zmk_display_work_q(), &refresh_work, REFRESH_PERIOD);
}

void ring_show_main(void) {
    if (s_root == NULL) {
        return;
    }

    s_shown = false;
    k_work_cancel_delayable(&refresh_work);

    lv_obj_del(s_root);
    s_root = NULL;
    for (int i = 0; i < 4; i++) {
        s_bar_bg[i] = NULL;
        s_bar_fill[i] = NULL;
        s_value[i] = NULL;
        s_reset[i] = NULL;
        s_col_label[i] = NULL;
    }
    s_tool[0] = s_tool[1] = NULL;
    s_ai_label = s_hdiv = s_vdiv = NULL;

    /* Restore the Main layout. */
    ring_battery_rings_apply_layout(false);
    ring_modifier_chips_apply_layout(false);
    ring_keys_info_set_visible(true);
    ring_brightness_info_set_visible(true);
    ring_status_screen_set_main_decor_visible(true);
}

bool ring_ai_usage_is_shown(void) {
    return s_shown;
}

void ring_ai_usage_apply_theme(void) {
    if (!s_shown) {
        return;
    }

    uint32_t track = ring_color_track();
    uint32_t text_sec = ring_color_text_sec();

    if (s_hdiv) {
        lv_obj_set_style_bg_color(s_hdiv, lv_color_hex(track), LV_PART_MAIN);
    }
    if (s_vdiv) {
        lv_obj_set_style_bg_color(s_vdiv, lv_color_hex(track), LV_PART_MAIN);
    }
    if (s_ai_label) {
        lv_obj_set_style_text_color(s_ai_label, lv_color_hex(text_sec), LV_PART_MAIN);
    }
    for (int i = 0; i < 4; i++) {
        if (s_bar_bg[i]) {
            lv_obj_set_style_bg_color(s_bar_bg[i], lv_color_hex(track), LV_PART_MAIN);
        }
        if (s_col_label[i]) {
            lv_obj_set_style_text_color(s_col_label[i], lv_color_hex(text_sec), LV_PART_MAIN);
        }
        if (s_reset[i]) {
            lv_obj_set_style_text_color(s_reset[i], lv_color_hex(text_sec), LV_PART_MAIN);
        }
    }
}

/* ── Toggle (any thread) ─────────────────────────────────────────────── */

static void toggle_async_cb(void *data) {
    ARG_UNUSED(data);
    if (s_root != NULL) {
        ring_show_main();
    } else {
        ring_show_ai_usage();
    }
}

void ring_ai_usage_toggle(void) {
    lv_async_call(toggle_async_cb, NULL);
}

/* ── Optional single-press keycode toggle ────────────────────────────── */

static int ring_ai_usage_init(void) {
    k_work_init_delayable(&refresh_work, refresh_work_cb);
    return 0;
}

SYS_INIT(ring_ai_usage_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#if IS_ENABLED(CONFIG_PROSPECTOR_RING_AI_USAGE_TOGGLE_KEY)

static int ai_usage_toggle_key_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || ev->usage_page != HID_USAGE_KEY ||
        ev->keycode != CONFIG_PROSPECTOR_RING_AI_USAGE_TOGGLE_KEYCODE) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* Toggle on key press (single press); ignore the release event. */
    if (ev->state) {
        ring_ai_usage_toggle();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ring_ai_usage_toggle_key, ai_usage_toggle_key_listener);
ZMK_SUBSCRIPTION(ring_ai_usage_toggle_key, zmk_keycode_state_changed);

#endif /* CONFIG_PROSPECTOR_RING_AI_USAGE_TOGGLE_KEY */

#endif /* CONFIG_PROSPECTOR_RING_AI_USAGE */

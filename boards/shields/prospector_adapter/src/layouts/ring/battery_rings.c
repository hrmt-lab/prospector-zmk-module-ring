#include "battery_rings.h"

#include <ctype.h>
#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>

#include "display_colors.h"

extern lv_font_t FR_Regular_36;
extern lv_font_t FR_Regular_30;
extern lv_font_t DINishCondensed_SemiBold_20;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// Ring center in absolute screen coordinates
#define RING_CENTER_X 96
#define RING_CENTER_Y 124

// Per-peripheral-count ring parameters
#if RING_PERIPHERAL_COUNT == 1
static const uint8_t  RING_RADII[]  = {78};
static const uint8_t  RING_STROKE   = 5;
static const uint32_t RING_COLORS[] = {RING_COLOR_P1};
#define LAYER_LABEL_Y   102
#define LAYER_NAME_Y    134
#define BATT_ROW_Y      156
#define BATT_VAL_Y      152
#define LAYER_NAME_FONT FR_Regular_36
#elif RING_PERIPHERAL_COUNT == 2
static const uint8_t  RING_RADII[]  = {78, 62};
static const uint8_t  RING_STROKE   = 4;
static const uint32_t RING_COLORS[] = {RING_COLOR_P1, RING_COLOR_P2};
#define LAYER_LABEL_Y   102
#define LAYER_NAME_Y    134
#define BATT_ROW_Y      156
#define BATT_VAL_Y      152
#define LAYER_NAME_FONT FR_Regular_36
#else
static const uint8_t  RING_RADII[]  = {78, 64, 50};
static const uint8_t  RING_STROKE   = 4;
static const uint32_t RING_COLORS[] = {RING_COLOR_P1, RING_COLOR_P2, RING_COLOR_P3};
#define LAYER_LABEL_Y   108
#define LAYER_NAME_Y    138
#define BATT_ROW_Y      158
#define BATT_VAL_Y      154
#define LAYER_NAME_FONT FR_Regular_30
#endif

// Per-peripheral dot/value positions (dot center x, dot center y, value label left x, value label right x)
#if RING_PERIPHERAL_COUNT == 1
static const int16_t DOT_CX[]    = {RING_CENTER_X - 12};
static const int16_t DOT_CY[]    = {BATT_ROW_Y};
static const int16_t VAL_LEFT[]  = {RING_CENTER_X - 12};
static const int16_t VAL_RIGHT[] = {RING_CENTER_X + 12};
#elif RING_PERIPHERAL_COUNT == 2
static const int16_t DOT_CX[]    = {RING_CENTER_X - 30, RING_CENTER_X + 6};
static const int16_t DOT_CY[]    = {BATT_ROW_Y, BATT_ROW_Y};
static const int16_t VAL_LEFT[]  = {RING_CENTER_X - 30, RING_CENTER_X + 6};
static const int16_t VAL_RIGHT[] = {RING_CENTER_X - 6,  RING_CENTER_X + 30};
#else
static const int16_t DOT_CX[]    = {RING_CENTER_X - 44, RING_CENTER_X - 12, RING_CENTER_X + 20};
static const int16_t DOT_CY[]    = {BATT_ROW_Y, BATT_ROW_Y, BATT_ROW_Y};
static const int16_t VAL_LEFT[]  = {RING_CENTER_X - 44, RING_CENTER_X - 12, RING_CENTER_X + 20};
static const int16_t VAL_RIGHT[] = {RING_CENTER_X - 32, RING_CENTER_X,      RING_CENTER_X + 32};
#endif

// Module-level LVGL object pointers (single widget instance assumed)
static lv_obj_t *s_arcs[RING_PERIPHERAL_COUNT];
static lv_obj_t *s_dots[RING_PERIPHERAL_COUNT];
static lv_obj_t *s_vals[RING_PERIPHERAL_COUNT];
static lv_obj_t *s_layer_name = NULL;

// Peripheral state cache
static uint8_t s_batt_level[RING_PERIPHERAL_COUNT];
static bool    s_connected[RING_PERIPHERAL_COUNT];

// ──────────────────────────────────────────────────────────
// Display update helpers
// ──────────────────────────────────────────────────────────

static void update_ring(uint8_t idx) {
    if (idx >= RING_PERIPHERAL_COUNT) {
        return;
    }
    bool    connected = s_connected[idx];
    uint8_t level     = s_batt_level[idx];

    lv_obj_t *arc = s_arcs[idx];
    if (arc) {
        lv_arc_set_value(arc, connected ? level : 0);
        lv_obj_set_style_arc_color(
            arc,
            lv_color_hex(connected ? RING_COLORS[idx] : RING_COLOR_TRACK),
            LV_PART_INDICATOR);
    }

    lv_obj_t *dot = s_dots[idx];
    if (dot) {
        lv_obj_set_style_bg_color(
            dot,
            lv_color_hex(connected ? RING_COLORS[idx] : RING_COLOR_TRACK),
            LV_PART_MAIN);
    }

    lv_obj_t *val = s_vals[idx];
    if (val) {
        if (!connected) {
            lv_label_set_text(val, "-");
        } else {
            lv_label_set_text_fmt(val, "%d", level);
        }
    }
}

// ──────────────────────────────────────────────────────────
// Battery event listener
// ──────────────────────────────────────────────────────────

struct batt_rings_batt_state {
    uint8_t source;
    uint8_t level;
};

static void batt_rings_batt_cb(struct batt_rings_batt_state state) {
    struct zmk_widget_battery_rings *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (state.source < RING_PERIPHERAL_COUNT) {
            s_batt_level[state.source] = state.level;
            update_ring(state.source);
        }
    }
}

static struct batt_rings_batt_state batt_rings_get_batt(const zmk_event_t *eh) {
    if (eh == NULL) {
        return (struct batt_rings_batt_state){.source = 0, .level = 0};
    }
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (ev == NULL) {
        return (struct batt_rings_batt_state){.source = 0, .level = 0};
    }
    return (struct batt_rings_batt_state){
        .source = ev->source,
        .level  = ev->state_of_charge,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_rings_batt, struct batt_rings_batt_state,
                            batt_rings_batt_cb, batt_rings_get_batt);
ZMK_SUBSCRIPTION(widget_battery_rings_batt, zmk_peripheral_battery_state_changed);

// ──────────────────────────────────────────────────────────
// Connection event listener
// ──────────────────────────────────────────────────────────

struct batt_rings_conn_state {
    uint8_t source;
    bool    connected;
};

static void batt_rings_conn_cb(struct batt_rings_conn_state state) {
    struct zmk_widget_battery_rings *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (state.source < RING_PERIPHERAL_COUNT) {
            s_connected[state.source] = state.connected;
            update_ring(state.source);
        }
    }
}

static struct batt_rings_conn_state batt_rings_get_conn(const zmk_event_t *eh) {
    if (eh == NULL) {
        return (struct batt_rings_conn_state){.source = 0, .connected = false};
    }
    const struct zmk_split_central_status_changed *ev =
        as_zmk_split_central_status_changed(eh);
    if (ev == NULL) {
        return (struct batt_rings_conn_state){.source = 0, .connected = false};
    }
    return (struct batt_rings_conn_state){
        .source    = ev->slot,
        .connected = ev->connected,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_rings_conn, struct batt_rings_conn_state,
                            batt_rings_conn_cb, batt_rings_get_conn);
ZMK_SUBSCRIPTION(widget_battery_rings_conn, zmk_split_central_status_changed);

// ──────────────────────────────────────────────────────────
// Layer event listener
// ──────────────────────────────────────────────────────────

struct batt_rings_layer_state {
    uint8_t index;
};

static void batt_rings_layer_cb(struct batt_rings_layer_state state) {
    if (!s_layer_name) {
        return;
    }

    const char *name = zmk_keymap_layer_name(state.index);
    if (name == NULL) {
        // fallback: "L0", "L1", ...
        lv_label_set_text_fmt(s_layer_name, "L%d", state.index);
        return;
    }

#if IS_ENABLED(CONFIG_PROSPECTOR_LAYER_NAME_UPPERCASE)
    // Convert to uppercase in place using a local buffer
    static char upper_buf[32];
    size_t i;
    for (i = 0; i < sizeof(upper_buf) - 1 && name[i]; i++) {
        upper_buf[i] = toupper((unsigned char)name[i]);
    }
    upper_buf[i] = '\0';
    lv_label_set_text(s_layer_name, upper_buf);
#else
    lv_label_set_text(s_layer_name, name);
#endif
}

static struct batt_rings_layer_state batt_rings_get_layer(const zmk_event_t *eh) {
    return (struct batt_rings_layer_state){
        .index = zmk_keymap_highest_layer_active(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_rings_layer, struct batt_rings_layer_state,
                            batt_rings_layer_cb, batt_rings_get_layer);
ZMK_SUBSCRIPTION(widget_battery_rings_layer, zmk_layer_state_changed);

// ──────────────────────────────────────────────────────────
// Arc creation helper
// ──────────────────────────────────────────────────────────

static lv_obj_t *create_ring_arc(lv_obj_t *parent, uint8_t radius, uint8_t stroke,
                                  uint32_t ring_color) {
    int size = radius * 2;
    int pos_x = RING_CENTER_X - radius;
    int pos_y = RING_CENTER_Y - radius;

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, pos_x, pos_y);

    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 50);   // Phase 1 fixed value; updated dynamically in Phase 2
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_rotation(arc, 270);

    // Background arc = track (gray)
    lv_obj_set_style_arc_width(arc, stroke, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(RING_COLOR_TRACK), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

    // Indicator arc = battery fill
    lv_obj_set_style_arc_width(arc, stroke, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(ring_color), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    return arc;
}

// ──────────────────────────────────────────────────────────
// Widget init / obj
// ──────────────────────────────────────────────────────────

int zmk_widget_battery_rings_init(struct zmk_widget_battery_rings *widget, lv_obj_t *parent) {
    widget->obj = parent;

    // ── Rings (outermost first) ──────────────────────────────
    int num_rings = RING_PERIPHERAL_COUNT > 3 ? 3 : RING_PERIPHERAL_COUNT;
    for (int i = 0; i < num_rings; i++) {
        s_arcs[i] = create_ring_arc(parent, RING_RADII[i], RING_STROKE, RING_COLORS[i]);
        widget->arcs[i] = s_arcs[i];
    }

    // ── LAYER label ─────────────────────────────────────────
    lv_obj_t *layer_label = lv_label_create(parent);
    lv_label_set_text(layer_label, "LAYER");
    lv_obj_set_style_text_color(layer_label, lv_color_hex(RING_COLOR_TEXT_SEC), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(layer_label, 2, LV_PART_MAIN);
    lv_obj_set_width(layer_label, 80);
    lv_obj_set_style_text_align(layer_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(layer_label, RING_CENTER_X - 40, LAYER_LABEL_Y - 5);

    // ── Layer name (serif placeholder) ──────────────────────
    s_layer_name = lv_label_create(parent);
    lv_label_set_text(s_layer_name, "Base");
    lv_obj_set_style_text_font(s_layer_name, &LAYER_NAME_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_layer_name, lv_color_hex(RING_COLOR_TEXT_PRI), LV_PART_MAIN);
    lv_obj_set_style_text_align(s_layer_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(s_layer_name, 150);
    lv_obj_set_pos(s_layer_name, RING_CENTER_X - 75, LAYER_NAME_Y - 20);
    widget->layer_name = s_layer_name;

    // ── Battery dots and value labels ────────────────────────
    for (int i = 0; i < num_rings; i++) {
        // Dot (6×6 circle)
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_set_size(dot, 6, 6);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(dot, lv_color_hex(RING_COLORS[i]), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(dot, 0, LV_PART_MAIN);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(dot, DOT_CX[i] - 3, DOT_CY[i] - 3);
        s_dots[i]          = dot;
        widget->batt_dots[i] = dot;

        // Value label (right-aligned within [dot_center_x, val_right_x])
        int16_t lbl_left  = VAL_LEFT[i];
        int16_t lbl_width = VAL_RIGHT[i] - VAL_LEFT[i];
        if (lbl_width < 20) {
            lbl_width = 20;
        }
        lv_obj_t *val = lv_label_create(parent);
        lv_label_set_text(val, "50");   // Phase 1 placeholder
        lv_obj_set_style_text_font(val, &DINishCondensed_SemiBold_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(val, lv_color_hex(RING_COLOR_TEXT_PRI), LV_PART_MAIN);
        lv_label_set_long_mode(val, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(val, lbl_width);
        lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_set_pos(val, lbl_left, BATT_VAL_Y);
        lv_obj_set_style_border_width(val, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(val, LV_OPA_TRANSP, LV_PART_MAIN);
        s_vals[i]             = val;
        widget->batt_values[i] = val;
    }

    // ── Register event listeners ─────────────────────────────
    widget_battery_rings_batt_init();
    widget_battery_rings_conn_init();
    widget_battery_rings_layer_init();

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

lv_obj_t *zmk_widget_battery_rings_obj(struct zmk_widget_battery_rings *widget) {
    return widget->obj;
}

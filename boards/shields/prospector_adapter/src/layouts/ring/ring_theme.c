#include "ring_theme.h"

#include <lvgl.h>
#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEY)
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#endif

#include "battery_rings.h"
#include "brightness_info.h"
#include "keys_info.h"
#include "modifier_chips.h"
#include "uptime_info.h"

/* Forward declaration: status_screen.c has no header of its own. */
void ring_status_screen_apply_theme(void);

/* ── Theme state ────────────────────────────────────────────────────── */

bool ring_dark_mode = false;

bool ring_theme_is_dark(void) { return ring_dark_mode; }

/* ── Apply helpers ──────────────────────────────────────────────────── */

void ring_theme_apply_all(void) {
    ring_status_screen_apply_theme();
    ring_battery_rings_apply_theme();
    ring_brightness_info_apply_theme();
    ring_keys_info_apply_theme();
    ring_modifier_chips_apply_theme();
    ring_uptime_info_apply_theme();
}

/* Called on the LVGL display thread via lv_async_call().
 * State flip and style updates happen atomically on the same thread,
 * so there is no window where ring_dark_mode and rendered colors disagree. */
static void toggle_theme_async_cb(void *data) {
    ring_dark_mode = !ring_dark_mode;
    ring_theme_apply_all();
}

void ring_theme_toggle(void) {
    /* lv_async_call is safe to invoke from any thread. */
    lv_async_call(toggle_theme_async_cb, NULL);
}

/* ── Optional keycode toggle ────────────────────────────────────────── */

#if IS_ENABLED(CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEY)

static int dark_toggle_key_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev && ev->state &&
        ev->usage_page == HID_USAGE_KEY &&
        ev->keycode    == CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEYCODE) {
        ring_theme_toggle();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ring_dark_toggle_key, dark_toggle_key_listener);
ZMK_SUBSCRIPTION(ring_dark_toggle_key, zmk_keycode_state_changed);

#endif /* CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEY */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <dt-bindings/zmk/hid_usage_pages.h>
#include <prospector_brightness.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

/*
 * This file intentionally keeps the Prospector idle timeout separate from ZMK's
 * global activity timeout. ZMK_IDLE_TIMEOUT may also be used by sleep/backlight
 * features, while this module only owns the Prospector display panel and its
 * backlight. A value of 0 for CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT leaves the
 * auto-off path out of the build, but key brightness control can still be used.
 */
#if CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT > 0
static void display_idle_timeout_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(display_idle_timeout_work, display_idle_timeout_cb);
#endif

static void display_wake(void) {
    /*
     * All callers can safely ask for wake. Keeping this idempotent makes it
     * harmless for both normal key activity and brightness-control keys to pass
     * through here, even if the display is already on.
     */
    if (prospector_brightness_is_screen_on()) {
        return;
    }

    /*
     * display_blanking_off() wakes the ST7789 panel itself. The brightness
     * helper restores the PWM backlight level separately, because Zephyr's
     * display blanking API does not know about Prospector's backlight pin.
     */
    if (device_is_ready(display)) {
        display_blanking_off(display);
    }
    prospector_brightness_screen_on();
}

#if CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT > 0
static void display_idle_schedule(void) {
    /*
     * Rescheduling the same delayable work gives us a compact debounce-style
     * idle timer: every key activity pushes the timeout out by the configured
     * number of seconds.
     */
    k_work_reschedule(&display_idle_timeout_work,
                      K_SECONDS(CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT));
}

static void display_idle_timeout_cb(struct k_work *work) {
    ARG_UNUSED(work);

    /*
     * The timeout can race with a wake event already handled on another path.
     * Checking screen_on here avoids sending duplicate blanking/PWM writes.
     */
    if (!prospector_brightness_is_screen_on()) {
        return;
    }

    /*
     * Turn off both halves of the visible display:
     * - display_blanking_on(): tells the ST7789 to stop showing pixels.
     * - prospector_brightness_screen_off(): sets the backlight PWM to 0.
     */
    if (device_is_ready(display)) {
        display_blanking_on(display);
    }
    prospector_brightness_screen_off();
}
#else
static void display_idle_schedule(void) {}
#endif

static void note_activity(void) {
    /*
     * Any physical key activity should wake the screen and restart the idle
     * countdown. We subscribe to position_state_changed for this path because it
     * tracks physical key activity without depending on which keycode the key
     * eventually produces.
     */
    display_wake();
    display_idle_schedule();
}

#if IS_ENABLED(CONFIG_PROSPECTOR_BRIGHTNESS_KEY_CONTROL)
static bool handle_brightness_key(const struct zmk_keycode_state_changed *ev) {
    /*
     * Brightness control should only run once per key press. keycode_state_changed
     * is raised for press and release, so ignore releases. Also ignore non-keyboard
     * usage pages so mouse/media events cannot accidentally match these numeric
     * keycode values.
     */
    if (ev == NULL || !ev->state || ev->usage_page != HID_USAGE_KEY) {
        return false;
    }

    if (ev->keycode == CONFIG_PROSPECTOR_BRIGHTNESS_UP_KEYCODE) {
        /*
         * Brightness keys are also a reasonable wake source. Waking first means
         * the user sees the result immediately if they press F23/F24 while the
         * display is idle-off.
         */
        display_wake();
        prospector_brightness_adjust_user_level(CONFIG_PROSPECTOR_BRIGHTNESS_STEP);
        display_idle_schedule();
        return true;
    }

    if (ev->keycode == CONFIG_PROSPECTOR_BRIGHTNESS_DOWN_KEYCODE) {
        display_wake();
        prospector_brightness_adjust_user_level(-CONFIG_PROSPECTOR_BRIGHTNESS_STEP);
        display_idle_schedule();
        return true;
    }

    return false;
}
#endif

static int display_idle_listener(const zmk_event_t *eh) {
#if IS_ENABLED(CONFIG_PROSPECTOR_BRIGHTNESS_KEY_CONTROL)
    /*
     * Handle brightness keys before generic activity. This lets brightness keys
     * adjust the backlight and then reset the idle timer exactly once.
     */
    const struct zmk_keycode_state_changed *key_ev = as_zmk_keycode_state_changed(eh);
    if (handle_brightness_key(key_ev)) {
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

#if CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT > 0
    if (as_zmk_position_state_changed(eh) != NULL) {
        note_activity();
    }
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(prospector_display_idle, display_idle_listener);

#if CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT > 0
ZMK_SUBSCRIPTION(prospector_display_idle, zmk_position_state_changed);
#endif

#if IS_ENABLED(CONFIG_PROSPECTOR_BRIGHTNESS_KEY_CONTROL)
ZMK_SUBSCRIPTION(prospector_display_idle, zmk_keycode_state_changed);
#endif

#if CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT > 0
static int display_idle_init(void) {
    /*
     * Start counting from boot once the application is initialized. The first
     * key activity will reschedule this work, so no separate "last activity"
     * timestamp is needed.
     */
    display_idle_schedule();
    return 0;
}

SYS_INIT(display_idle_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

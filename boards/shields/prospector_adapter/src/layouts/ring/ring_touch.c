#include "ring_theme.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/dt-bindings/input/cst816s-gesture-codes.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_PROSPECTOR_RING_GESTURE_NAV
#include "ring_nav.h"
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* CST816S register addresses (not exported by the upstream driver). */
#define CST816S_REG_MOTION_MASK 0xEC
#define CST816S_REG_IRQ_CTL     0xFA
#define CST816S_MOTION_EN_CON_LR BIT(2)
#define CST816S_MOTION_EN_CON_UR BIT(1)
#define CST816S_MOTION_EN_DCLICK BIT(0)
#define CST816S_IRQ_EN_MOTION    BIT(4)

#define CST816S_MOTION_MASK_GESTURES \
    (CST816S_MOTION_EN_CON_LR | CST816S_MOTION_EN_CON_UR | CST816S_MOTION_EN_DCLICK)

/* Enable CST816S motion gestures.
 * The upstream driver only enables TOUCH/CHANGE IRQs; we must additionally
 * set MOTION_MASK and IRQ_EN_MOTION to receive gesture events.           */
static int ring_touch_init(void) {
    static const struct i2c_dt_spec cst_i2c =
        I2C_DT_SPEC_GET(DT_NODELABEL(cst816s));

    if (!device_is_ready(cst_i2c.bus)) {
        LOG_ERR("ring_touch: I2C bus not ready");
        return -ENODEV;
    }

    int ret;
    /* Enable gestures in the motion mask register. */
    ret = i2c_reg_update_byte_dt(&cst_i2c,
        CST816S_REG_MOTION_MASK, CST816S_MOTION_MASK_GESTURES, CST816S_MOTION_MASK_GESTURES);
    if (ret < 0) {
        LOG_WRN("ring_touch: failed to set MOTION_MASK (%d)", ret);
        return ret;
    }

    /* Enable motion IRQ so gesture events fire the interrupt line. */
    ret = i2c_reg_update_byte_dt(&cst_i2c,
        CST816S_REG_IRQ_CTL, CST816S_IRQ_EN_MOTION, CST816S_IRQ_EN_MOTION);
    if (ret < 0) {
        LOG_WRN("ring_touch: failed to set IRQ_CTL (%d)", ret);
        return ret;
    }

    LOG_INF("ring_touch: CST816S gestures enabled");
    return 0;
}
/* Run after CST816S driver (POST_KERNEL) has initialized the chip. */
SYS_INIT(ring_touch_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* Receive Zephyr input events from the CST816S driver. */
#ifdef CONFIG_PROSPECTOR_RING_GESTURE_NAV
static int16_t s_touch_x = 0;
static int16_t s_touch_y = 0;
static int16_t s_current_x = 0;
static int16_t s_current_y = 0;
static int64_t s_last_swipe_ms = 0;

#define SCREEN_W 280
#define SCREEN_H 240
#define TOUCH_PHYS_W SCREEN_H
#define TOUCH_PHYS_H SCREEN_W
#define SWIPE_COOLDOWN_MS 350

static int16_t clamp_coord(int16_t value, int16_t max) {
    if (value < 0) {
        return 0;
    }
    if (value >= max) {
        return max - 1;
    }
    return value;
}

static void touch_to_screen_point(int16_t *x, int16_t *y) {
    int16_t raw_x = *x;
    int16_t raw_y = *y;

#ifdef CONFIG_PROSPECTOR_ROTATE_DISPLAY_180
    /* Current rotate config selects DISPLAY_ORIENTATION_ROTATED_90. */
    *x = clamp_coord(TOUCH_PHYS_H - raw_y, SCREEN_W);
    *y = clamp_coord(raw_x, SCREEN_H);
#else
    /* Default display orientation is DISPLAY_ORIENTATION_ROTATED_270. */
    *x = clamp_coord(raw_y, SCREEN_W);
    *y = clamp_coord(TOUCH_PHYS_W - raw_x, SCREEN_H);
#endif
}

static void update_current_screen_point(void) {
    s_current_x = s_touch_x;
    s_current_y = s_touch_y;
    touch_to_screen_point(&s_current_x, &s_current_y);
}

static uint16_t gesture_to_screen_direction(uint16_t gesture) {
#ifdef CONFIG_PROSPECTOR_ROTATE_DISPLAY_180
    /* Current rotate config selects DISPLAY_ORIENTATION_ROTATED_90. */
    switch (gesture) {
    case CST816S_GESTURE_CODE_SWIPE_UP:
        return CST816S_GESTURE_CODE_SWIPE_RIGHT;
    case CST816S_GESTURE_CODE_SWIPE_DOWN:
        return CST816S_GESTURE_CODE_SWIPE_LEFT;
    case CST816S_GESTURE_CODE_SWIPE_LEFT:
        return CST816S_GESTURE_CODE_SWIPE_UP;
    case CST816S_GESTURE_CODE_SWIPE_RIGHT:
        return CST816S_GESTURE_CODE_SWIPE_DOWN;
    default:
        return gesture;
    }
#else
    /* Default display orientation is DISPLAY_ORIENTATION_ROTATED_270. */
    switch (gesture) {
    case CST816S_GESTURE_CODE_SWIPE_UP:
        return CST816S_GESTURE_CODE_SWIPE_RIGHT;
    case CST816S_GESTURE_CODE_SWIPE_DOWN:
        return CST816S_GESTURE_CODE_SWIPE_LEFT;
    case CST816S_GESTURE_CODE_SWIPE_LEFT:
        return CST816S_GESTURE_CODE_SWIPE_DOWN;
    case CST816S_GESTURE_CODE_SWIPE_RIGHT:
        return CST816S_GESTURE_CODE_SWIPE_UP;
    default:
        return gesture;
    }
#endif
}

static bool swipe_cooldown_active(void) {
    int64_t now = k_uptime_get();

    if ((now - s_last_swipe_ms) < SWIPE_COOLDOWN_MS) {
        return true;
    }

    s_last_swipe_ms = now;
    return false;
}

static void handle_gesture_swipe(uint16_t gesture) {
    if (swipe_cooldown_active()) {
        return;
    }

    switch (gesture_to_screen_direction(gesture)) {
    case CST816S_GESTURE_CODE_SWIPE_LEFT:
        ring_nav_swipe_left();
        break;
    case CST816S_GESTURE_CODE_SWIPE_RIGHT:
        ring_nav_swipe_right();
        break;
    default:
        break;
    }
}
#endif

static void touch_input_cb(struct input_event *evt, void *user_data) {
    ARG_UNUSED(user_data);

#ifdef CONFIG_PROSPECTOR_RING_GESTURE_NAV
    if (evt->type == INPUT_EV_ABS) {
        if (evt->code == INPUT_ABS_X) {
            s_touch_x = (int16_t)evt->value;
        } else if (evt->code == INPUT_ABS_Y) {
            s_touch_y = (int16_t)evt->value;
        }
        update_current_screen_point();
        return;
    }

    if (evt->type == INPUT_EV_KEY && evt->code == INPUT_BTN_TOUCH) {
        update_current_screen_point();
        return;
    }
#endif

    if (evt->type != INPUT_EV_DEVICE) {
        return;
    }

    switch (evt->code) {
    case CST816S_GESTURE_CODE_DOUBLE_CLICK:
#ifdef CONFIG_PROSPECTOR_RING_DARK_TOGGLE_TOUCH
        ring_theme_toggle();
#endif
        break;
#ifdef CONFIG_PROSPECTOR_RING_GESTURE_NAV
    case CST816S_GESTURE_CODE_SWIPE_LEFT:
    case CST816S_GESTURE_CODE_SWIPE_RIGHT:
    case CST816S_GESTURE_CODE_SWIPE_UP:
    case CST816S_GESTURE_CODE_SWIPE_DOWN:
        handle_gesture_swipe((uint16_t)evt->code);
        break;
    case CST816S_GESTURE_CODE_SINGLE_CLICK:
        ring_nav_handle_tap(s_current_x, s_current_y);
        break;
#endif
    default:
        break;
    }
}
INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(cst816s)), touch_input_cb, NULL);

#include "ring_theme.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/cst816s-gesture-codes.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_PROSPECTOR_RING_GESTURE_NAV
#include "ring_nav.h"
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* CST816S register addresses (not exported by the upstream driver). */
#define CST816S_REG_MOTION_MASK 0xEC
#define CST816S_REG_IRQ_CTL     0xFA
#define CST816S_MOTION_EN_DCLICK BIT(0)
#define CST816S_IRQ_EN_MOTION    BIT(4)

/* Enable double-click motion detection on the CST816S chip.
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
    /* Enable double-click in the motion mask register. */
    ret = i2c_reg_update_byte_dt(&cst_i2c,
        CST816S_REG_MOTION_MASK, CST816S_MOTION_EN_DCLICK, CST816S_MOTION_EN_DCLICK);
    if (ret < 0) {
        LOG_WRN("ring_touch: failed to set MOTION_MASK (%d)", ret);
        return ret;
    }

    /* Enable motion IRQ so double-click fires the interrupt line. */
    ret = i2c_reg_update_byte_dt(&cst_i2c,
        CST816S_REG_IRQ_CTL, CST816S_IRQ_EN_MOTION, CST816S_IRQ_EN_MOTION);
    if (ret < 0) {
        LOG_WRN("ring_touch: failed to set IRQ_CTL (%d)", ret);
        return ret;
    }

    LOG_INF("ring_touch: CST816S double-click enabled");
    return 0;
}
/* Run after CST816S driver (POST_KERNEL) has initialized the chip. */
SYS_INIT(ring_touch_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* Receive Zephyr input events from the CST816S driver. */
#ifdef CONFIG_PROSPECTOR_RING_GESTURE_NAV
static int16_t s_touch_x = 0;
static int16_t s_touch_y = 0;
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
        return;
    }
#endif

    if (evt->type != INPUT_EV_DEVICE) {
        return;
    }

    switch (evt->code) {
    case CST816S_GESTURE_CODE_DOUBLE_CLICK:
        ring_theme_toggle();
        break;
#ifdef CONFIG_PROSPECTOR_RING_GESTURE_NAV
    case CST816S_GESTURE_CODE_SWIPE_LEFT:
        ring_nav_swipe_left();
        break;
    case CST816S_GESTURE_CODE_SWIPE_RIGHT:
        ring_nav_swipe_right();
        break;
    case CST816S_GESTURE_CODE_SINGLE_CLICK:
        ring_nav_handle_tap(s_touch_x, s_touch_y);
        break;
#endif
    default:
        break;
    }
}
INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(cst816s)), touch_input_cb, NULL);

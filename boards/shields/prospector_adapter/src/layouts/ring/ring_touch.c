#include "ring_theme.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/cst816s-gesture-codes.h>
#include <zephyr/logging/log.h>

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
SYS_INIT(ring_touch_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY + 1);

/* Receive Zephyr input events from the CST816S driver. */
static void touch_input_cb(struct input_event *evt) {
    if (evt->type == INPUT_EV_DEVICE &&
        evt->code == CST816S_GESTURE_CODE_DOUBLE_CLICK) {
        ring_theme_toggle();
    }
}
INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(cst816s)), touch_input_cb);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/printk.h>

#include <prospector_brightness.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(als, 4);

static const struct device *pwm_leds_dev = DEVICE_DT_GET_ONE(pwm_leds);
#define DISP_BL DT_NODE_CHILD_IDX(DT_NODELABEL(disp_bl))

/*
 * Brightness state is centralized here so the ALS loop, fixed-brightness mode,
 * idle display-off, and key brightness controls all agree on whether PWM writes
 * should actually reach the backlight.
 */
static bool screen_on = true;
static uint8_t applied_brightness = 0;

static uint8_t clamp_brightness(uint8_t value) {
    if (value > 100) {
        return 100;
    }
    if (value < 1) {
        return 1;
    }
    return value;
}

static int apply_brightness(uint8_t value) {
    /*
     * Keep the raw LED call in one place. This makes it easier to ensure idle-off
     * sets the PWM to 0 and that normal brightness changes update our cached
     * applied value.
     */
    int ret = led_set_brightness(pwm_leds_dev, DISP_BL, value);
    if (ret) {
        LOG_ERR("Failed to set brightness");
        return ret;
    }

    applied_brightness = value;
    return 0;
}

#ifdef CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR

static uint8_t current_brightness = 100;
static uint8_t user_brightness = 100;

#define SENSOR_MIN      0       // Minimum sensor reading
#define SENSOR_MAX      100   // Maximum sensor reading
#define PWM_MIN         1       // Minimum PWM duty cycle (%) - keep display visible
#define PWM_MAX         100     // Maximum PWM duty cycle (%)

#define FADE_STEP                        1
#define FADE_SLEEP_BRIGHTEN_MS           3
#define FADE_SLEEP_DARKEN_MS             10
#define FADE_THRESHOLD                   10

#define NORMAL_SAMPLE_SLEEP_MS           100

#define BURST_SAMPLE_SLEEP_MS            30
#define BURST_SAMPLE_TIMEOUT             10
#define BURST_SAMPLE_CONSECUTIVE         3

static uint8_t effective_brightness(void) {
    /*
     * In ALS mode the light sensor decides the desired brightness, while the user
     * brightness set by F23/F24 behaves as a cap. This preserves automatic dimming
     * in dark rooms but still lets the user lower the maximum comfortable level.
     */
    return current_brightness < user_brightness ? current_brightness : user_brightness;
}

uint8_t map_light_to_pwm(int32_t sensor_reading) {
    // Handle invalid/error readings
    if (sensor_reading < SENSOR_MIN) {
        return PWM_MIN;  // Default to minimum brightness on error
    }

    // Clamp to maximum
    if (sensor_reading > SENSOR_MAX) {
        sensor_reading = SENSOR_MAX;
    }

    // Linear mapping
    uint8_t pwm_value = (uint8_t)(
        PWM_MIN + ((PWM_MAX - PWM_MIN) *
        (sensor_reading - SENSOR_MIN)) / (SENSOR_MAX - SENSOR_MIN)
    );

    return pwm_value;
}

uint8_t bl_fade(uint8_t source, uint8_t target) {
    bool increasing = target > source;
    int brightness = source;

    while ((increasing && brightness < target) ||
           (!increasing && brightness > target)) {
        /*
         * If the idle manager turns the screen off while a fade is in progress,
         * stop immediately and force PWM to 0. Otherwise the fade loop could
         * continue writing nonzero values after auto-off.
         */
        if (!screen_on) {
            apply_brightness(0);
            return 0;
        }

        apply_brightness((uint8_t)brightness);
        brightness += increasing ? FADE_STEP : -FADE_STEP;

        if (brightness > 100) {
            brightness = 100;
        } else if (brightness < 0) {
            brightness = 0;
        }

        k_msleep(increasing ? FADE_SLEEP_BRIGHTEN_MS : FADE_SLEEP_DARKEN_MS);
    }

    if (screen_on) {
        apply_brightness(target);
    }

    return 0;
}

extern void als_thread(void *d0, void *d1, void *d2) {
    ARG_UNUSED(d0);
    ARG_UNUSED(d1);
    ARG_UNUSED(d2);

    const struct device *dev;
    struct sensor_value intensity;
    uint8_t mapped_brightness;

    dev = DEVICE_DT_GET_ONE(avago_apds9960);
    if (!device_is_ready(dev)) {
        /*
         * Without a working sensor there is nothing to poll. Apply a visible
         * default brightness once and stop the thread instead of fetching from
         * an unready device every NORMAL_SAMPLE_SLEEP_MS forever.
         */
        LOG_ERR("ALS sensor not ready; applying default brightness and stopping ALS loop");
        apply_brightness(effective_brightness());
        return;
    }

    /*
     * ALS mode used to begin with the in-memory default brightness. Apply the
     * effective value explicitly so the cached applied_brightness value matches
     * the hardware before the first sensor-driven fade.
     */
    apply_brightness(effective_brightness());

    while (1) {

        k_msleep(NORMAL_SAMPLE_SLEEP_MS);


        if (sensor_sample_fetch(dev)) {
            LOG_ERR("sensor_sample fetch failed\n");
        }

        if (sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &intensity)) {
            LOG_ERR("Cannot read ALS data.\n");
        }

        // LOG_INF("ambient light intensity %d", intensity.val1);

        mapped_brightness = map_light_to_pwm(intensity.val1);
        // LOG_INF("NORMAL: mapped PWM duty cycle %d\n", mapped_brightness);

        if (abs(mapped_brightness - current_brightness) > FADE_THRESHOLD) {
            uint8_t integrator = 0;

            for (int i = 0; i < BURST_SAMPLE_TIMEOUT; i++) {
                k_msleep(BURST_SAMPLE_SLEEP_MS);

                if (sensor_sample_fetch(dev)) {
                    LOG_ERR("sensor_sample fetch failed\n");
                }
                if (sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &intensity)) {
                    LOG_ERR("Cannot read ALS data.\n");
                }

                mapped_brightness = map_light_to_pwm(intensity.val1);
                // LOG_INF("BURST: mapped PWM duty cycle %d\n", mapped_brightness);

                if (abs(mapped_brightness - current_brightness) > FADE_THRESHOLD) {
                    integrator++;
                    // printk("integrator at: %d", integrator);
                    if (integrator >= BURST_SAMPLE_CONSECUTIVE) {
                        current_brightness = mapped_brightness;
                        /*
                         * Keep reading ALS while the screen is off so the next
                         * wake uses a fresh brightness, but do not write PWM
                         * while idle-off is active.
                         */
                        if (screen_on) {
                            bl_fade(applied_brightness, effective_brightness());
                        }
                        // LOG_INF("SETTING NEW BRIGHTNESS: %d", mapped_brightness);
                        break;
                    }
                }
            }
        }
        // led_set_brightness(pwm_leds_dev, DISP_BL, map_light_to_pwm(intensity.val1));
    }
}

K_THREAD_DEFINE(als_tid, 1024, als_thread, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0,
                0);

#else

static uint8_t user_brightness = CONFIG_PROSPECTOR_FIXED_BRIGHTNESS;

static int init_fixed_brightness(void) {
    /*
     * In fixed mode the user brightness starts from the configured fixed value
     * and can later be adjusted by the optional key brightness controls.
     */
    apply_brightness(user_brightness);

    return 0;
}

SYS_INIT(init_fixed_brightness, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif

void prospector_brightness_screen_off(void) {
    /*
     * This is called by display_idle.c after the ST7789 has been blanked. PWM 0
     * is what actually makes the physical backlight go dark.
     */
    screen_on = false;
    apply_brightness(0);
}

void prospector_brightness_screen_on(void) {
    uint8_t target;

    screen_on = true;
#ifdef CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR
    /*
     * Resume with the latest ALS-derived value, capped by any user brightness
     * adjustment made through the configured keycodes.
     */
    target = effective_brightness();
#else
    target = user_brightness;
#endif
    apply_brightness(target);
}

bool prospector_brightness_is_screen_on(void) {
    return screen_on;
}

void prospector_brightness_set_user_level(uint8_t level) {
    user_brightness = clamp_brightness(level);
    if (screen_on) {
#ifdef CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR
        /*
         * In ALS mode the key changes the cap, so fade from the currently applied
         * PWM to the newly capped effective value.
         */
        bl_fade(applied_brightness, effective_brightness());
#else
        apply_brightness(user_brightness);
#endif
    }
}

void prospector_brightness_adjust_user_level(int8_t delta) {
    int level = user_brightness + delta;

    /*
     * Clamp manual brightness between 1 and 100. A value of 0 is reserved for
     * screen-off state, so repeatedly pressing brightness-down will leave the
     * display visible at minimum brightness rather than turning it off manually.
     */
    if (level > 100) {
        level = 100;
    } else if (level < 1) {
        level = 1;
    }

    prospector_brightness_set_user_level((uint8_t)level);
}

uint8_t prospector_brightness_get_user_level(void) {
    return user_brightness;
}

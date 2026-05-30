#pragma once

#include <stdbool.h>

/* AI Usage screen controller.
 *
 * The AI Usage screen reuses the Main screen's shared LVGL objects (keyboard
 * name, uptime, MOD/IME chips, layer name, brightness) by repositioning them,
 * and builds its own root container for the provider usage bars.  All entry
 * points below must run on the LVGL display thread; ring_ai_usage_toggle()
 * schedules the switch via lv_async_call() and is safe to call from any
 * context. */

/* Hold duration (ms) before a key/touch long-press toggles the screen.
 * The toggle fires once this threshold elapses while still held (not on
 * release). */
#define RING_AI_USAGE_LONGPRESS_MS 700

void ring_show_ai_usage(void);
void ring_show_main(void);

/* Toggle between Main and AI Usage (safe from any thread). */
void ring_ai_usage_toggle(void);

bool ring_ai_usage_is_shown(void);

/* Re-apply theme-dependent colors to AI Usage objects (no-op when not shown). */
void ring_ai_usage_apply_theme(void);

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ring_dark_mode is defined in ring_theme.c.  Exposed as extern so the
 * inline accessors below can read it without an extra function call.    */
extern bool ring_dark_mode;

bool ring_theme_is_dark(void);

/* Schedule a theme toggle on the LVGL thread.  Safe to call from any
 * context (ZMK event handlers, touch ISR, etc.).                        */
void ring_theme_toggle(void);

/* Apply current theme to all RING widgets.  Must be called on the LVGL
 * display thread (it is called automatically from ring_theme_toggle).   */
void ring_theme_apply_all(void);

/* ── Theme-dependent color accessors ─────────────────────────────────
 * Returns a uint32_t 0xRRGGBB value matching the current theme.
 * Pass the result to lv_color_hex().                                    */
static inline uint32_t ring_color_bg(void)
    { return ring_dark_mode ? 0x22282Cu : 0xFAFAFAu; }

static inline uint32_t ring_color_text_pri(void)
    { return ring_dark_mode ? 0xE9E7E1u : 0x22282Cu; }

static inline uint32_t ring_color_text_sec(void)
    { return ring_dark_mode ? 0x929FA7u : 0x5F6A70u; }

/* text_off: chip / indicator OFF-state label color */
static inline uint32_t ring_color_text_off(void)
    { return ring_dark_mode ? 0x3A4248u : 0xC6CDD1u; }

static inline uint32_t ring_color_track(void)
    { return ring_dark_mode ? 0x3A4248u : 0xE2E5E8u; }

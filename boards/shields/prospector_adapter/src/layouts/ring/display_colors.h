#pragma once

/* Static (theme-invariant) color constants.
 * Theme-dependent colors (bg, text_pri, text_sec, text_off, track) are
 * accessed via the ring_color_XXX() inline functions in ring_theme.h.    */
#define RING_COLOR_BG        0xFAFAFA   /* light-theme default, kept for reference */
#define RING_COLOR_TEXT_PRI  0x22282C
#define RING_COLOR_TEXT_SEC  0x5F6A70
#define RING_COLOR_TEXT_TER  0x929FA7   /* label tertiary – same in both themes */
#define RING_COLOR_TEXT_OFF  0xD8DCDF   /* chip OFF label (light); dark = 0x3A4248 */
#define RING_COLOR_ACCENT    0xE89B5C
#define RING_COLOR_P1        0x7A8B92
#define RING_COLOR_P2        0xA6B2B8
#define RING_COLOR_P3        0xC4CCD1
#define RING_COLOR_TRACK     0xE2E5E8   /* light-theme default, kept for reference */

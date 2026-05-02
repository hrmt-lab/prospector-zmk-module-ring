#pragma once

#include <stdbool.h>
#include <stdint.h>

void prospector_brightness_screen_off(void);
void prospector_brightness_screen_on(void);
bool prospector_brightness_is_screen_on(void);
void prospector_brightness_set_user_level(uint8_t level);
void prospector_brightness_adjust_user_level(int8_t delta);


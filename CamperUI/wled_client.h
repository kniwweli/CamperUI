#pragma once
#include <Arduino.h>

void wled_set_power(bool on);
void wled_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);
void wled_set_brightness(uint8_t bri); // 0..255
void wled_set_white(uint8_t w);
void wled_set_state(bool on, uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t bri);

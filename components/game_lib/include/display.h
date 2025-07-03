#pragma once
#include <stdint.h>
#include <stddef.h>

void display_init(void);

void display_fill_screen(uint16_t color);
void display_draw_rect(uint16_t x, uint16_t y,
                       uint16_t w, uint16_t h,
                       uint16_t color);
void display_draw_rgb565(uint16_t x, uint16_t y,
                         const uint8_t* rgb565, size_t bytes);


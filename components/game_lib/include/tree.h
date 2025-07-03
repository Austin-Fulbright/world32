#pragma once
#include <stdint.h>

#define TREE_1_FRAME_COUNT   1
#define TREE_1_FRAME_WIDTH   64
#define TREE_1_FRAME_HEIGHT  64
#define IMAGE_SIZE  (TREE_1_FRAME_WIDTH * TREE_1_FRAME_HEIGHT)

// scratch buffer for RGB565 data (2 bytes per pixel)
extern uint8_t rgb565_buffer_global[IMAGE_SIZE * 2];

void tree_1_to_rgb565(void);

void tree_1_draw(uint16_t x, uint16_t y);


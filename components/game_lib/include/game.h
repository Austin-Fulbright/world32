#pragma once
#include <stdint.h>

typedef struct {
    int16_t x, y;
    int16_t w, h;
} Obstacle;

void game_init(const Obstacle* obstacles, int num_obstacles);

void game_step(void);


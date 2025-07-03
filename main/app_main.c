#include <stdio.h>
#include "game.h"    

void app_main(void)
{
	static const Obstacle s_obstacles[] = {
		{ 50, 50, 20, 10 },
		{100, 80, 30, 15}
	};
	game_init(s_obstacles, sizeof(s_obstacles)/sizeof(s_obstacles[0]));
    while (1) {
		game_step();
    }
}


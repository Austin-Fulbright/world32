#include "game.h"
#include "display.h"
#include "joystick.h"
#include "mqtt_game.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_OTHER_PLAYERS 10

typedef struct {
    char id[32];
    int16_t x, y;
} OtherPlayer;

static const Obstacle* s_obstacles;
static int s_num_obstacles;

static OtherPlayer s_players[MAX_OTHER_PLAYERS];
static int s_player_count;

static int16_t s_x, s_y, s_prev_x, s_prev_y;
static const int16_t s_square_size = 20;
static const int16_t s_dead_zone = 200;
static const int16_t s_max_speed = 5;
static const uint16_t s_bg_color = 0xFFFF;
static const uint16_t s_player_color = 0xF800;
static const uint16_t s_square_color_default = 0x0000;

static void on_other_player(const char* id, int16_t x, int16_t y) {
    for (int i = 0; i < s_player_count; i++) {
        if (strcmp(s_players[i].id, id)==0) {
            s_players[i].x = x;
            s_players[i].y = y;
            return;
        }
    }
    if (s_player_count < MAX_OTHER_PLAYERS) {
        strncpy(s_players[s_player_count].id, id, 31);
        s_players[s_player_count].x = x;
        s_players[s_player_count].y = y;
        s_player_count++;
    }
}

void game_init(const Obstacle* obstacles, int num_obstacles) {
    s_obstacles = obstacles;
    s_num_obstacles = num_obstacles;
    s_player_count = 0;

    display_init();
    joystick_init();
    mqtt_game_init("mqtt://broker.hivemq.com",
                  "esp32/game/player1/position",
                  "esp32/game/+/position");
    mqtt_game_set_player_callback(on_other_player);

    display_fill_screen(s_bg_color);
    for (int i = 0; i < s_num_obstacles; i++) {
        display_draw_rect(obstacles[i].x,
                          obstacles[i].y,
                          obstacles[i].w,
                          obstacles[i].h,
                          0x07E0);
    }

    s_x = 0;  s_y = 150;
    s_prev_x = s_x;  s_prev_y = s_y;
    display_draw_rect(s_x, s_y,
                      s_square_size, s_square_size,
                      s_square_color_default);
}

void game_step(void) {
    int joy_x = joystick_read_x();
    int joy_y = joystick_read_y();
    if (abs(joy_x) < s_dead_zone) joy_x = 0;
    if (abs(joy_y) < s_dead_zone) joy_y = 0;
    int dx = (joy_x * s_max_speed) / 2048;
    int dy = (joy_y * s_max_speed) / 2048;

    s_x += dx;  s_y += dy;
    if (s_x < 0) s_x = 0;
    if (s_x > 240 - s_square_size) s_x = 240 - s_square_size;
    if (s_y < 0) s_y = 0;
    if (s_y > 320 - s_square_size) s_y = 320 - s_square_size;

    display_draw_rect(s_prev_x, s_prev_y,
                      s_square_size, s_square_size,
                      s_bg_color);

    display_draw_rect(s_x, s_y,
                      s_square_size, s_square_size,
                      joystick_read_sw()==0 ? 0xF800 : 0x0000);

    mqtt_game_publish(s_x, s_y);

    for (int i = 0; i < s_player_count; i++) {
        display_draw_rect(s_players[i].x,
                          s_players[i].y,
                          s_square_size,
                          s_square_size,
                          s_player_color);
    }

    s_prev_x = s_x;
    s_prev_y = s_y;

    vTaskDelay(pdMS_TO_TICKS(20));
}


#pragma once
#include <stdint.h>
void mqtt_game_init(const char* broker_uri,
                    const char* pub_topic,
                    const char* sub_topic_wildcard);

void mqtt_game_publish(int16_t x, int16_t y);

typedef void (*other_player_cb_t)(const char* player_id,
                                  int16_t x, int16_t y);
void mqtt_game_set_player_callback(other_player_cb_t cb);


#include <string.h>
#include "mqtt_game.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

static const char* TAG = "mqtt_game";
static esp_mqtt_client_handle_t client = NULL;
static other_player_cb_t user_cb = NULL;
static const char* s_pub_topic = NULL;
static const char* s_sub_topic = NULL;

static void handle_data(const char* topic, const char* data, int len)
{
    char copy[128];
    strncpy(copy, topic, sizeof(copy)-1);
    copy[sizeof(copy)-1] = '\0';

    // expect topic format “esp32/game/playerX”
    char* tok = strtok(copy, "/");
    tok = strtok(NULL, "/");       // “game”
    tok = strtok(NULL, "/");       // “playerX”
    if (!tok) return;
    const char* player_id = tok;

    cJSON* root = cJSON_ParseWithLength(data, len);
    if (!root) return;
    cJSON* x = cJSON_GetObjectItem(root, "x");
    cJSON* y = cJSON_GetObjectItem(root, "y");
    if (cJSON_IsNumber(x) && cJSON_IsNumber(y) && user_cb) {
        user_cb(player_id, x->valueint, y->valueint);
    }
    cJSON_Delete(root);
}

static void mqtt_event_cb(void* handler_args,
                         esp_event_base_t base,
                         int32_t event_id,
                         void* event_data)
{
    esp_mqtt_event_handle_t evt = event_data;

    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected, subscribing to %s", s_sub_topic);
        esp_mqtt_client_subscribe(client, s_sub_topic, 0);
        break;
    case MQTT_EVENT_DATA:
        handle_data(evt->topic, evt->data, evt->data_len);
        break;
    default:
        break;
    }
}

void mqtt_game_init(const char* uri,
                    const char* pub_topic,
                    const char* sub_topic)
{
    s_pub_topic = pub_topic;
    s_sub_topic = sub_topic;

    esp_mqtt_client_config_t cfg = {
        .broker = {
            .address = {
                .uri = uri
            }
        }
    };
    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client,
                                   MQTT_EVENT_ANY,
                                   mqtt_event_cb,
                                   NULL);
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "MQTT init: broker=%s pub=%s sub=%s",
             uri, s_pub_topic, s_sub_topic);
}

void mqtt_game_publish(int16_t x, int16_t y)
{
    if (!client) return;
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "x", x);
    cJSON_AddNumberToObject(root, "y", y);
    char* msg = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(client,
                            s_pub_topic,
                            msg,
                            strlen(msg),
                            1,
                            0);
    cJSON_Delete(root);
    free(msg);
}

void mqtt_game_set_player_callback(other_player_cb_t cb)
{
    user_cb = cb;
}


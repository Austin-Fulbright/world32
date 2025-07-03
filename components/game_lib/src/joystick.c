#include "joystick.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char* TAG = "joystick";
static adc_oneshot_unit_handle_t adc1_handle;

#define PIN_JOY_X GPIO_NUM_3 // ADC1_CHANNEL_2
#define PIN_JOY_Y GPIO_NUM_4 // ADC1_CHANNEL_3
#define PIN_JOY_SW GPIO_NUM_5

void joystick_init(void) {
    // ADC init
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &chan_cfg));

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<PIN_JOY_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Initialized");
}

int joystick_read_x(void) {
    int raw;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &raw);
    return raw - 2048;
}

int joystick_read_y(void) {
    int raw;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &raw);
    return raw - 2048;
}

int joystick_read_sw(void) {
    return gpio_get_level(PIN_JOY_SW);
}


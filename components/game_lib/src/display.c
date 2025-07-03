#include "display.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "display";

#define PIN_NUM_MOSI 11
#define PIN_NUM_MISO -1
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   10
#define PIN_NUM_DC    9
#define PIN_NUM_RST  48

#define SPI_HOST_BUS    SPI2_HOST
#define SPI_DMA_CHAN    SPI_DMA_CH_AUTO
#define SPI_CHUNK_SIZE 1024

static spi_device_handle_t spi = NULL;

static void send_cmd(uint8_t cmd) {
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(spi, &t);
}

static void send_data(const uint8_t* data, int len) {
    if (len == 0) return;
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(spi, &t);
}

static void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t buf[4];
    send_cmd(0x2A); // Column addr set
    buf[0] = x0>>8; buf[1] = x0; buf[2] = x1>>8; buf[3] = x1;
    send_data(buf,4);

    send_cmd(0x2B); // Page addr set
    buf[0] = y0>>8; buf[1] = y0; buf[2] = y1>>8; buf[3] = y1;
    send_data(buf,4);

    send_cmd(0x2C); // Memory write
}

void display_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<PIN_NUM_DC)|(1ULL<<PIN_NUM_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    // SPI bus init
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240*2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_BUS, &buscfg, SPI_DMA_CHAN));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40*1000*1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_BUS, &devcfg, &spi));

    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    send_cmd(0x01); vTaskDelay(pdMS_TO_TICKS(100)); // SW reset
    send_cmd(0x29);  // Display ON
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_LOGI(TAG, "Initialized");
}

void display_fill_screen(uint16_t color) {
    set_address_window(0, 0, 239, 319);
    uint8_t line[240*2];
    for (int i = 0; i < 240; i++) {
        line[2*i]   = color >> 8;
        line[2*i+1] = color & 0xFF;
    }
    for (int y = 0; y < 320; y++) {
        send_data(line, sizeof(line));
    }
}

void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    set_address_window(x, y, x + w - 1, y + h - 1);
    int area = w * h;
    int chunk = SPI_CHUNK_SIZE;
    uint8_t buf[chunk*2];
    for (int i = 0; i < chunk; i++) {
        buf[2*i]   = color >> 8;
        buf[2*i+1] = color & 0xFF;
    }
    while (area > 0) {
        int send = (area > chunk) ? chunk : area;
        send_data(buf, send * 2);
        area -= send;
    }
}

void display_draw_rgb565(uint16_t x, uint16_t y,
                         const uint8_t* rgb565, size_t bytes) {
    size_t sent = 0;
    while (sent < bytes) {
        size_t to_send = (bytes - sent > SPI_CHUNK_SIZE)
                         ? SPI_CHUNK_SIZE
                         : (bytes - sent);
        send_data(&rgb565[sent], to_send);
        sent += to_send;
    }
}


// components/game_lib/src/display.c
#include <stddef.h>
#include <string.h>
#include "display.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "display";

#define PIN_NUM_MOSI    11
#define PIN_NUM_MISO    -1
#define PIN_NUM_CLK     12
#define PIN_NUM_CS      10
#define PIN_NUM_DC       9
#define PIN_NUM_RST     48

#define SPI_HOST_BUS    SPI2_HOST
#define SPI_DMA_CHAN    SPI_DMA_CH_AUTO

#define WIDTH   240
#define HEIGHT  320
#define FB_SIZE (WIDTH * HEIGHT * 2)  // bytes per frame

static uint8_t fb0[FB_SIZE] __attribute__((aligned(4), section(".dma")));
static uint8_t fb1[FB_SIZE] __attribute__((aligned(4), section(".dma")));
static uint8_t *active_fb = fb0;
static uint8_t *sending_fb = fb1;

static spi_device_handle_t spi = NULL;

static void send_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = {
        .length    = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(spi, &t);
}

static void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t buf[4];

    send_cmd(0x2A);            // Column addr set
    buf[0] = x0 >> 8; buf[1] = x0 & 0xFF;
    buf[2] = x1 >> 8; buf[3] = x1 & 0xFF;
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t_col = {
        .length    = 32,
        .tx_buffer = buf,
    };
    spi_device_transmit(spi, &t_col);

    send_cmd(0x2B);            // Page addr set
    buf[0] = y0 >> 8; buf[1] = y0 & 0xFF;
    buf[2] = y1 >> 8; buf[3] = y1 & 0xFF;
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t_page = {
        .length    = 32,
        .tx_buffer = buf,
    };
    spi_device_transmit(spi, &t_page);

    send_cmd(0x2C);            // Memory write
}

void display_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<PIN_NUM_DC)|(1ULL<<PIN_NUM_RST),
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = FB_SIZE,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_BUS, &buscfg, SPI_DMA_CHAN));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40*1000*1000,
        .mode           = 0,
        .spics_io_num   = PIN_NUM_CS,
        .queue_size     = 2,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_BUS, &devcfg, &spi));
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    send_cmd(0x01);  // SW reset
    vTaskDelay(pdMS_TO_TICKS(100));
    send_cmd(0x29);  // Display ON
    vTaskDelay(pdMS_TO_TICKS(20));

    ESP_LOGI(TAG, "ILI9341 initialized, %ux%u, double-buffered", WIDTH, HEIGHT);
}

static void display_flush(void)
{
    set_address_window(0, 0, WIDTH-1, HEIGHT-1);

    spi_transaction_t t = {
        .length    = FB_SIZE * 8,   // bits
        .tx_buffer = sending_fb,
    };
    ESP_ERROR_CHECK(spi_device_queue_trans(spi, &t, portMAX_DELAY));

    spi_transaction_t *r;
    ESP_ERROR_CHECK(spi_device_get_trans_result(spi, &r, portMAX_DELAY));

    uint8_t *tmp = sending_fb;
    sending_fb = active_fb;
    active_fb  = tmp;
}

void display_fill_screen(uint16_t color)
{
    uint32_t color32 = ((uint32_t)color << 16) | color;
    uint32_t *p32    = (uint32_t*)active_fb;
    size_t count32   = FB_SIZE / 4;
    for (size_t i = 0; i < count32; i++) {
        p32[i] = color32;
    }
    display_flush();
}

void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint32_t color32 = ((uint32_t)color << 16) | color;
    for (uint16_t row = y; row < y + h; row++) {
        uint8_t *rowptr = active_fb + ((size_t)row * WIDTH + x) * 2;
        // write 32-bit chunks
        uint32_t *p32   = (uint32_t*)rowptr;
        size_t  chunks  = (w * 2) / 4;
        for (size_t i = 0; i < chunks; i++) {
            p32[i] = color32;
        }
        // handle any trailing 2 bytes
        if ((w & 1) != 0) {
            uint8_t *tail = rowptr + chunks * 4;
            tail[0] = color >> 8;
            tail[1] = color & 0xFF;
        }
    }
    display_flush();
}

void display_draw_rgb565(uint16_t x, uint16_t y,
                         const uint8_t* rgb565, size_t bytes)
{
    uint8_t *dst = active_fb + (((size_t)y * WIDTH + x) * 2);
    memcpy(dst, rgb565, bytes);
    display_flush();
}


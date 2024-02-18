/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#include <string.h>
#include "app_led.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/rmt.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "lvgl.h"
#include "app_drive.h"


static const char *TAG = "app_led";
static led_strip_t *strip = NULL;

uint8_t ws_rgb_buf[8][3];

static led_state_t s_led_state = {
    .on = false,
    .h = 170,
    .s = 100,
    .v = 30,
    .gpio = WS2812_PIN,
};

esp_err_t app_led_get_state(led_state_t *state)
{
    if (NULL == state) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(state, &s_led_state, sizeof(led_state_t));

    return ESP_OK;
}

esp_err_t app_led_init(gpio_num_t io_num)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(io_num, RMT_CHANNEL_0);
    config.clk_div = 2;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    led_strip_config_t strip_config = 
        LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(strip->clear(strip, 100));

    return ESP_OK;
}

esp_err_t app_led_deinit(void)
{
    if (NULL == strip) {
        ESP_LOGE(TAG, "LED not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret_val = led_strip_denit(strip);
    strip = NULL;

    return ret_val;
}

esp_err_t app_led_updata(void)
{
    esp_err_t ret_val = ESP_OK;

    if (NULL == strip) {
        ESP_LOGE(TAG, "LED not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < CONFIG_EXAMPLE_STRIP_LED_NUMBER; i++) {
        ret_val |= strip->set_pixel(strip, i, ws_rgb_buf[i][0], ws_rgb_buf[i][1], ws_rgb_buf[i][2]);
    }
    ret_val |= strip->refresh(strip, 0);

    return ret_val;
}

esp_err_t app_led_right(void)
{
    uint8_t rgb[3];

    rgb[0] = ws_rgb_buf[7][0];
    rgb[1] = ws_rgb_buf[7][1];
    rgb[2] = ws_rgb_buf[7][2];
    for(int i=0; i<7; i++){
        ws_rgb_buf[7-i][0] = ws_rgb_buf[6-i][0];
        ws_rgb_buf[7-i][1] = ws_rgb_buf[6-i][1];
        ws_rgb_buf[7-i][2] = ws_rgb_buf[6-i][2];
    }
    ws_rgb_buf[0][0] = rgb[0];
    ws_rgb_buf[0][1] = rgb[1];
    ws_rgb_buf[0][2] = rgb[2];

    return app_led_updata();
}

esp_err_t app_led_left(void)
{
    uint8_t rgb[3];

    rgb[0] = ws_rgb_buf[0][0];
    rgb[1] = ws_rgb_buf[0][1];
    rgb[2] = ws_rgb_buf[0][2];
    for(int i=0; i<7; i++){
        ws_rgb_buf[i][0] = ws_rgb_buf[i+1][0];
        ws_rgb_buf[i][1] = ws_rgb_buf[i+1][1];
        ws_rgb_buf[i][2] = ws_rgb_buf[i+1][2];
    }
    ws_rgb_buf[7][0] = rgb[0];
    ws_rgb_buf[7][1] = rgb[1];
    ws_rgb_buf[7][2] = rgb[2];

    return app_led_updata();
}

esp_err_t app_led_set_all(uint8_t red, uint8_t green, uint8_t blue)
{
    for(int i=0; i<8; i++){
        ws_rgb_buf[i][0] = red;
        ws_rgb_buf[i][1] = green;
        ws_rgb_buf[i][2] = blue;
    }

    return app_led_updata();
}


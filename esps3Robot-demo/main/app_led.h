/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_EXAMPLE_STRIP_LED_NUMBER (8)

extern uint8_t ws_rgb_buf[8][3];

typedef struct {
    uint16_t h;
    uint8_t s;
    uint8_t v;
    bool    on;
    gpio_num_t gpio;
} led_state_t;

esp_err_t app_led_init(gpio_num_t io_num);
esp_err_t app_led_set_all(uint8_t red, uint8_t green, uint8_t blue);
esp_err_t app_led_get_state(led_state_t *state);
esp_err_t app_led_left(void);
esp_err_t app_led_right(void);
esp_err_t app_led_updata(void);


#ifdef __cplusplus
}
#endif

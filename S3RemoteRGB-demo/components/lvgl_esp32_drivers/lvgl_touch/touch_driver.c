/**
 * @file touch_driver.c
 */

#include "touch_driver.h"
#include "lv_port_indev.h"


void touch_driver_init(void)
{
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400000));
    lv_port_indev_init();
}


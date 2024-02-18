#ifndef _OV5640_FOCUS_H
#define _OV5640_FOCUS_H

#include "freertos/FreeRTOS.h"
#include "esp_system.h"


uint8_t Camera_Focus_Init(void);
void camera_af_s(void);
void camera_af_c(void);
void camera_af_off(void);




#endif
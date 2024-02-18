/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#ifndef MATERIALS_CLASSIFIER_ESP_APP_CAMERA_ESP_H_
#define MATERIALS_CLASSIFIER_ESP_APP_CAMERA_ESP_H_

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sensor.h"
#include "../myBoard.h"

/**
 * PIXFORMAT_RGB565,    // 2BPP/RGB565
 * PIXFORMAT_YUV422,    // 2BPP/YUV422
 * PIXFORMAT_GRAYSCALE, // 1BPP/GRAYSCALE
 * PIXFORMAT_JPEG,      // JPEG/COMPRESSED
 * PIXFORMAT_RGB888,    // 3BPP/RGB888
 */
#define CAMERA_PIXEL_FORMAT PIXFORMAT_RGB565

/*
 * FRAMESIZE_96X96,    // 96x96
 * FRAMESIZE_QQVGA,    // 160x120
 * FRAMESIZE_QQVGA2,   // 128x160
 * FRAMESIZE_QCIF,     // 176x144
 * FRAMESIZE_HQVGA,    // 240x176
 * FRAMESIZE_240X240    // 240x240
 * FRAMESIZE_QVGA,     // 320x240
 * FRAMESIZE_CIF,      // 400x296
 * FRAMESIZE_VGA,      // 640x480
 * FRAMESIZE_SVGA,     // 800x600
 * FRAMESIZE_XGA,      // 1024x768
 * FRAMESIZE_SXGA,     // 1280x1024
 * FRAMESIZE_UXGA,     // 1600x1200
 */
#define CAMERA_FRAME_SIZE FRAMESIZE_QVGA
    #ifdef JSZ_ROBOT_medium
    #define CAM_PIN_PWDN -1
    #define CAM_PIN_RESET -1
    #define CAM_PIN_XCLK -1
    #define CAM_PIN_SIOD 39
    #define CAM_PIN_SIOC 38

    #define CAM_PIN_D7 21
    #define CAM_PIN_D6 13
    #define CAM_PIN_D5 12
    #define CAM_PIN_D4 10
    #define CAM_PIN_D3 8
    #define CAM_PIN_D2 17
    #define CAM_PIN_D1 18
    #define CAM_PIN_D0 9
    #define CAM_PIN_VSYNC 48
    #define CAM_PIN_HREF 47
    #define CAM_PIN_PCLK 11
    #define FLIP_CAMERA 1
#endif
#ifdef JSZ_ROBOT_mini
    #define CAM_PIN_PWDN -1
    #define CAM_PIN_RESET 47
    #define CAM_PIN_XCLK -1
    #define CAM_PIN_SIOD 14
    #define CAM_PIN_SIOC 21

    #define CAM_PIN_D7 35
    #define CAM_PIN_D6 37
    #define CAM_PIN_D5 38
    #define CAM_PIN_D4 40
    #define CAM_PIN_D3 42
    #define CAM_PIN_D2 1
    #define CAM_PIN_D1 2
    #define CAM_PIN_D0 41
    #define CAM_PIN_VSYNC 48
    #define CAM_PIN_HREF 45
    #define CAM_PIN_PCLK 39
    #define FLIP_CAMERA 1
#endif

#define XCLK_FREQ 20000000

#ifdef __cplusplus
extern "C"
{
#endif

    void app_camera_init();//初始化摄像头
#ifdef __cplusplus
}
#endif
#endif
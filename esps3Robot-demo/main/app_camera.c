/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#include "app_camera.h"
#include "app_drive.h"
#include "ov5640_focus.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_camera";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    //不起作用
    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
//    .fb_location = CAMERA_FB_IN_PSRAM,//存放在外部PSRAM中
    .fb_location = CAMERA_FB_IN_DRAM,
    .pixel_format = PIXFORMAT_JPEG,//PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_VGA,     //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
                                      //     .frame_size = FRAMESIZE_240X240,
    .jpeg_quality = 30,               //0-63 lower number means higher quality
    .fb_count = 1,                    //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY};

static void camera_ledc_init(void)//XCLK
{
    //配置camera xclk时钟
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = ledc_timer_camera,
        .duty_resolution  = LEDC_TIMER_2_BIT,// Set duty resolution to 13 bits
        .freq_hz          = ledc_freq_timer0, // Frequency in Hertz.
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = ledc_channel_camera,
        .timer_sel      = ledc_timer_camera,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = ledc_pin_camera_xclk,
        .duty           = 1, // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

}
void app_camera_init()
{
    // camera init
    esps3_robot.CameraState = 0;
    camera_ledc_init();
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
    if(Camera_Focus_Init() == 0)
    {
        printf("focus Initialization complete.\r\n");
        camera_af_s();
    }
    //audioInit();
    esps3_robot.CameraState = 1;
}

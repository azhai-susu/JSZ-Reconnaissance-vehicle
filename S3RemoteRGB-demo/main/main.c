/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl_helpers.h"
#include "esp_freertos_hooks.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lvgl_touch/lv_port_indev.h"
#include "drive.h"
#include "ui/func.h"
#include "JPEGDecoder/JPEGDecoder.h"
#include "esp_timer.h"
#include "esp_mac.h"

static const char *TAG = "main";


nvs_handle my_nvs_handle;
uint32_t photoCount = 0;

extern uint8_t sdcard_init(void);
extern void renderJPEG(int xpos, int ypos);
extern void jpegInfo();
extern uint8_t file_save(uint8_t *buf, size_t len);
extern void sd_unmount(void);

extern uint8_t mouse_enable;

extern int fps_n;
extern const unsigned char pic1[];

SemaphoreHandle_t xGuiSemaphore;

uint32_t get_photoCount(void)
{
   photoCount ++;
   return photoCount;
}

static void lv_tick_task(void *arg)
{
   static uint8_t t1 = 0;
   (void)arg;
   lv_tick_inc(10);
   fps_n ++;
   t1 ++;
   if(t1 >= 10)
   {
      t1 = 0;
      get_keyboard();
   }
}

static void gui_task(void *arg)
{
	//Create and start a periodic timer interrupt to call lv_tick_inc
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &lv_tick_task,
		.name = "periodic_gui"};
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10 * 1000));
   //ui_init();
   while (1)
   {
      vTaskDelay(pdMS_TO_TICKS(10));
      if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
      {
         lv_timer_handler();
         xSemaphoreGive(xGuiSemaphore);
      }
   }
}


void app_main(void)
{
   // 初始化nvs用于存放wifi或者其他需要掉电保存的东西
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		printf("\r\nnvs_flash_erase\r\n\r\n");
		err = nvs_flash_init();
	}
   ESP_ERROR_CHECK(err);
   //设置热点名称和密码
   sprintf(esps3_remote.wifi_ap_ssid,WIFI_SSID);
   sprintf(esps3_remote.wifi_ap_pass,WIFI_PASS);
   sprintf(esps3_remote.password,"1234");
   printf("wifi_ap_ssid:%s, wifi_ap_pass:%s\n",esps3_remote.wifi_ap_ssid,esps3_remote.wifi_ap_pass);
   //初始化外设
   drive_init();
   //初始化LVGL显示模块
   xGuiSemaphore = xSemaphoreCreateMutex();
   lv_init();          //lvgl内核初始化
   lvgl_driver_init(); //lvgl显示接口初始化
   xTaskCreatePinnedToCore(gui_task, "gui task", 1024 * 4, NULL, 1, NULL, 0);
   //显示开机画面
   mouse_enable = 0;
   JPEGDecoder_decodeArray(pic1,15423);
   renderJPEG(0, 0);
   JPEGDecoder_abort();
   //初始化SD卡和WiFi
   sdcard_init();//初始化SD卡，如果要用串口调试需要注释掉并取下SD卡，与USB口复用
   wifi_init_ap();//作为AP连接

   while(1)
   {
      vTaskDelay(100);
   }
}

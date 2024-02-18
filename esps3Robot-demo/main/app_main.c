/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/

#include <stdio.h>

#include "lvgl_helpers.h"
#include "esp_freertos_hooks.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "app_camera.h"
#include "app_main.h"
#include "page_cam.h"

#include "string.h"

#include "driver/ledc.h"
#include "esp_err.h"

#include "app_network.h"
#include "app_drive.h"

#include "page_imu.h"
#include "mpu6050.h"

#include "ov5640_focus.h"
#include "app_led.h"
#include "wav_player.h"
#include "file_manager.h"


#define TAG "ESP32S3"


extern uint8_t mpu_ready;
extern int cam_verify_jpeg_soi(const uint8_t *inbuf, uint32_t length);

extern uint8_t net_connected; //连接状态

uint16_t camera_id = 0;
uint8_t led_state = 0;//0-关闭，1-常亮，2-熄灭，3-慢闪，4-快闪

float angle_x_last = 90;
float angle_y_last = 90;

camera_fb_t *fb = NULL;

lv_obj_t *label_my_name;
lv_obj_t *label_wifi_ssid;
lv_obj_t *label_wifi_pass;
lv_obj_t *label_local_ip;
lv_obj_t *label_remote_ip;
lv_obj_t *label_camera;
lv_obj_t *label_other;


lv_obj_t *cont_1;

//为LVGL提供时钟
static void lv_tick_task(void *arg)
{
	(void)arg;
	lv_tick_inc(10);
}

//监控CPU状态
void CPU_Task(void *arg)
{
	uint8_t cpu_run_info[400];
	uint8_t t1 = 0;
	uint8_t state_last = 0;
	char str1[5] = {0};
	char str2[5] = {0};

	//LVGL初始化
	lv_init(); // lvgl内核初始化
	lvgl_driver_init(); // lvgl显示接口初始化
	lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(DISP_BUF_SIZE * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(DISP_BUF_SIZE * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	
	static lv_disp_buf_t disp_buf;
	uint32_t size_in_px = DISP_BUF_SIZE;
	lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = disp_driver_flush;
	disp_drv.buffer = &disp_buf;
	lv_disp_drv_register(&disp_drv);

	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	//indev_drv.read_cb = touch_driver_read;
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	lv_indev_drv_register(&indev_drv);

	/* 创建一个定时器中断来进入 lv_tick_inc 给lvgl运行提供心跳 这里是10ms一次 主要是动画运行要用到 */
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &lv_tick_task,
		.name = "periodic_gui"};
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10 * 1000));;

	//设置样式
	static lv_style_t style_cont;
	lv_style_init(&style_cont);
	lv_style_set_radius(&style_cont, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_cont, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_grad_color(&style_cont, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_grad_dir(&style_cont, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_cont, LV_STATE_DEFAULT, 255);

	static lv_style_t style_label_a;
	lv_style_init(&style_label_a);
	lv_style_set_radius(&style_label_a, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_label_a, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_grad_color(&style_label_a, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_grad_dir(&style_label_a, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_label_a, LV_STATE_DEFAULT, 255);
	lv_style_set_text_color(&style_label_a, LV_STATE_DEFAULT, LV_COLOR_RED);

	static lv_style_t style_label_b;
	lv_style_init(&style_label_b);
	lv_style_set_radius(&style_label_b, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_label_b, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_grad_color(&style_label_b, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_grad_dir(&style_label_b, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_label_b, LV_STATE_DEFAULT, 255);
	lv_style_set_text_color(&style_label_b, LV_STATE_DEFAULT, LV_COLOR_RED);

	static lv_style_t style_labe2;
	lv_style_init(&style_labe2);
	lv_style_set_radius(&style_labe2, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_labe2, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_grad_color(&style_labe2, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_bg_grad_dir(&style_labe2, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_labe2, LV_STATE_DEFAULT, 255);
	lv_style_set_text_color(&style_labe2, LV_STATE_DEFAULT, LV_COLOR_WHITE);


	cont_1 = lv_cont_create(lv_scr_act(), NULL);
	lv_cont_set_fit2(cont_1, LV_FIT_NONE, LV_FIT_NONE);
	lv_obj_set_size(cont_1, 240, 240);
	lv_obj_set_pos(cont_1,0, 0);  
	lv_obj_add_style(cont_1, LV_LABEL_PART_MAIN, &style_cont);
	
	label_my_name = lv_label_create(cont_1, NULL);
	lv_obj_set_pos(label_my_name,4, 2);
	lv_obj_add_style(label_my_name, LV_LABEL_PART_MAIN, &style_labe2);
	lv_label_set_text(label_my_name, "NAME:");

	label_camera = lv_label_create(cont_1, NULL);
	lv_obj_set_pos(label_camera,4, 20);
	lv_obj_add_style(label_camera, LV_LABEL_PART_MAIN, &style_labe2);
	lv_label_set_text(label_camera, "CAMERA:");

	label_wifi_ssid = lv_label_create(cont_1, NULL);
	lv_obj_set_pos(label_wifi_ssid,4, 38);
	lv_obj_add_style(label_wifi_ssid, LV_LABEL_PART_MAIN, &style_labe2);
	lv_label_set_text(label_wifi_ssid, "WSSID:");

	label_wifi_pass = lv_label_create(cont_1, NULL);
	lv_obj_set_pos(label_wifi_pass,4, 56);
	lv_obj_add_style(label_wifi_pass, LV_LABEL_PART_MAIN, &style_labe2);
	lv_label_set_text(label_wifi_pass, "WPASS:");

	label_local_ip = lv_label_create(cont_1, NULL);
	lv_obj_set_pos(label_local_ip,4, 74);
	lv_obj_add_style(label_local_ip, LV_LABEL_PART_MAIN, &style_label_a);
	lv_label_set_text(label_local_ip, "No network!");

	label_remote_ip = lv_label_create(cont_1, NULL);
	lv_obj_set_pos(label_remote_ip,4, 92);
	lv_obj_add_style(label_remote_ip, LV_LABEL_PART_MAIN, &style_label_b);
	lv_label_set_text(label_remote_ip, "Not connected!");

	label_other = lv_label_create(cont_1, NULL);
	lv_obj_set_pos(label_other,4, 110);
	lv_obj_add_style(label_other, LV_LABEL_PART_MAIN, &style_labe2);
	lv_label_set_text(label_other, "MPU: MIC: BAT:");

	while(1)
	{
		t1 ++;
		if(t1>=200)
		{
			t1 = 0;
			//esps3_robot.electricityQuantity = get_bat_voltage();

			if(led_state == 100)//测试模式
			{
				
			}
			else if(led_state != 1)//非设置模式
			{
				lv_label_set_text_fmt(label_my_name, "NAME: %s", esps3_robot.name);
				lv_label_set_text_fmt(label_wifi_ssid, "WSSID: %s", esps3_robot.wifi_ssid);
				lv_label_set_text_fmt(label_wifi_pass, "WPASS: %s", esps3_robot.wifi_pass);
				if(esps3_robot.localIp[0] == 0)
				{
					lv_style_set_text_color(&style_label_a, LV_STATE_DEFAULT, LV_COLOR_RED);
					lv_label_set_text(label_local_ip, "No network!");
				}
				else
				{
					lv_style_set_text_color(&style_label_a, LV_STATE_DEFAULT, LV_COLOR_BLUE);
					lv_label_set_text_fmt(label_local_ip, "My IP: %s", esps3_robot.localIp);
				}
				if(esps3_robot.usageMode == 0)
				{
					lv_style_set_text_color(&style_label_b, LV_STATE_DEFAULT, LV_COLOR_RED);
					lv_label_set_text(label_remote_ip, "Not connected!");
				}
				else
				{
					lv_style_set_text_color(&style_label_b, LV_STATE_DEFAULT, LV_COLOR_BLUE);
					lv_label_set_text_fmt(label_remote_ip, "To IP: %s", esps3_robot.remoteIp);
				}
				lv_label_set_text_fmt(label_camera, "Camera: %X", camera_id);
				if(mpu_ready)
				{
					str1[0] = 'O';
					str1[1] = 'K';
				}
				else
				{
					str1[0] = 'N';
					str1[1] = 'O';
				}
				if(audio_ready)
				{
					str2[0] = 'O';
					str2[1] = 'K';
				}
				else
				{
					str2[0] = 'N';
					str2[1] = 'O';
				}
				lv_label_set_text_fmt(label_other, "MPU:%s  MIC:%s  BAT:%d%%", str1,str2,esps3_robot.electricityQuantity);
				//lv_label_set_text_fmt(label_other, "BAT:%d", esps3_robot.electricityQuantity);
			}
			else
			{
				lv_label_set_text(label_my_name, " ");
				lv_label_set_text(label_wifi_ssid, " ");
				lv_label_set_text(label_wifi_pass, " ");
				lv_style_set_text_color(&style_label_a, LV_STATE_DEFAULT, LV_COLOR_BLUE);
				lv_label_set_text(label_local_ip, "     Enter setting mode!");
				lv_label_set_text(label_remote_ip, " ");
				lv_label_set_text(label_camera, " ");
				lv_label_set_text(label_other, " ");
			}
			lv_task_handler();//LVGL刷新
		}
		if(t1%10 == 0)
		{
			//流水灯
			#ifdef JSZ_ROBOT_medium
			if(led_state == 4)
				app_led_right();
			#endif
		}
		if(t1%50 == 0)
		{
			//失联超时重启
			if(esps3_robot.usageMode > 0)
				time_r ++;
			if(time_r == 4)
				robotStop(); 
			else if(time_r > 20)
				esp_restart();

		}
		if(led_state)
		{
			if(state_last != led_state)
			{
				state_last = led_state;
				if(led_state == 1)
					gpio_set_level(KEY_PIN_SET, 0);
				if(led_state == 2)
					gpio_set_level(KEY_PIN_SET, 1);
			}
			if(led_state == 3)
			{
				if(t1 % 50 == 0)
					gpio_set_level(KEY_PIN_SET, 0);
				else if(t1 % 25 == 0)
					gpio_set_level(KEY_PIN_SET, 1);
			}
			if(led_state == 4)
			{
				if(t1 % 20 == 0)
					gpio_set_level(KEY_PIN_SET, 0);
				else if(t1 % 10 == 0)
					gpio_set_level(KEY_PIN_SET, 1);
			}
		}
		vTaskDelay(10);
	}
}
void app_main(void)
{
	//初始化机器人驱动
	robotDriveInit();

	#ifdef JSZ_ROBOT_medium
	app_led_init(WS2812_PIN);
	app_led_set_all(20, 20, 20);
	#endif

	// 初始化nvs用于存放wifi或者其他需要掉电保存的东西
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		printf("\r\nnvs_flash_erase\r\n\r\n");
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	audioInit();
	#ifdef JSZ_ROBOT_medium
	//初始化SPIFFS
	fm_spiffs_init();
	play_spiffs_name("Windows.wav");//播放wav
	#endif
	//初始化CPU监控程序
	xTaskCreatePinnedToCore(&CPU_Task, "CPU_Task", 1024 * 4, NULL, 3, NULL, 1);

	sprintf(esps3_robot.wifi_ssid,"%s", DEFAULT_ESP_WIFI_SSID);
	sprintf(esps3_robot.wifi_pass,"%s", DEFAULT_ESP_WIFI_PASS);
	sprintf(esps3_robot.name,"%s", DEFAULT_ESP_NAME);
	sprintf(esps3_robot.password,"%s", DEFAULT_ESP_PASS);
	// 打印机器人信息
	printf("\r\n----------information----------\r\n");
	printf("bilibili/头条/淘宝：技术宅物语\r\n");
	printf("version:V3.1\r\n\n");
	printf("wifi_ssid:%s\r\n",esps3_robot.wifi_ssid);
    printf("wifi_pass:%s\r\n",esps3_robot.wifi_pass);
    printf("robot_name:%s\r\n",esps3_robot.name);
    printf("robot_pass:%s\r\n",esps3_robot.password);
	printf("----------information----------\r\n");

	esps3_robot.localIp[0] = 0;
	esps3_robot.remoteIp[0] = 0;

	gpio_pad_select_gpio(KEY_PIN_SET);
	gpio_set_direction(KEY_PIN_SET, GPIO_MODE_OUTPUT_OD);
	gpio_set_pull_mode(KEY_PIN_SET, GPIO_FLOATING);
	led_state = 4;

	//MPU6050初始化
	//Imu_Task();

	//开辟PSRAM内存
    udpSendBuff = heap_caps_malloc(200000, MALLOC_CAP_SPIRAM);
    audioBuff1 = heap_caps_malloc(30000, MALLOC_CAP_SPIRAM);
    audioBuff2 = heap_caps_malloc(30000, MALLOC_CAP_SPIRAM);
	heap_show();
	//初始化摄像头
	page_cam_load();
	
	//连接WiFi
	#ifdef JSZ_ROBOT_medium
	for(int i=0; i<8; i++){
		ws_rgb_buf[i][0] = i*20;
		ws_rgb_buf[i][1] = (7-i)*10;
		ws_rgb_buf[i][2] = 20;
	}
	app_led_updata();
	#endif
	wifi_init_sta();
	#ifdef JSZ_ROBOT_medium
	for(int i=0; i<8; i++){
		ws_rgb_buf[i][0] = 0;
		ws_rgb_buf[i][1] = 0;
		ws_rgb_buf[i][2] = 0;
	}
	app_led_updata();
	#endif
	uint8_t net_connected_old = 0;
	while(1){
		#ifdef JSZ_ROBOT_medium
		if(net_connected_old != net_connected){
			net_connected_old = net_connected;
			if(net_connected_old)
				play_spiffs_name("wifi_con.wav");//播放wav
			else
				play_spiffs_name("wifi_disc.wav");//播放wav
		}
		#endif
		vTaskDelay(100);
	}
}

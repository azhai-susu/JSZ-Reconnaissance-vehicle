
#include "page_cam.h"
#include "app_main.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lvgl_helpers.h"
#include "lv_port_indev.h"
#include "app_camera.h"

#include <esp_system.h>
#include "esp_log.h"
#include "lv_port_indev.h"

#include "app_drive.h"

#define TAG "PAGE_CAM"



void page_cam_load()
{
	app_camera_init();//初始化摄像头
	if(esps3_robot.CameraState == 0)
		return;
	vTaskDelay(100);
	esps3_robot.CameraEnable = 0;
	esps3_robot.CameraState = 2;
	esps3_robot.getImgTo = 0;
}

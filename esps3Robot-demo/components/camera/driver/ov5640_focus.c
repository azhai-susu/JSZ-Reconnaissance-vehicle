
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "sensor.h"
#include "sccb.h"
#include "cam_hal.h"
#include "esp_camera.h"

#include "ov5640.h"
#include "ov5640af.h"
#include "ov5640_focus.h"

typedef struct {
    sensor_t sensor;
    camera_fb_t fb;
} camera_state_t;

extern camera_state_t *s_state;
extern uint16_t camera_id;

uint8_t Camera_Focus_Init(void)
{ 
	return 1;
}  

uint8_t OV5640_Focus_Single(void)
{
	
	return 0;	 		
}

uint8_t OV5640_Focus_Constant(void)
{
	
	return 0;
}

void OV5640_Focus_Off(void)
{
	
}

void camera_af_s(void)
{
	
}

void camera_af_c(void)
{

}

void camera_af_off(void)
{

}





#ifndef _page_cam_
#define _page_cam_

#ifdef __cplusplus
extern "C" {
#endif
/*********************
* INCLUDES
*********************/
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lvgl.h"
#include "lv_ex_conf.h"
#else
#include "../../lvgl/lvgl.h"
#endif
extern uint8_t cam_en, color_en, face_en;
void Cam_Task(void *pvParameters);
void page_cam_load(void);
void page_cam_end(void);

#ifdef __cplusplus
} /* extern "C" */
#endif




#endif // _TEST_



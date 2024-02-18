/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#define VERSION "0.9.0"

typedef enum
{
    WAIT_FOR_WAKEUP,
    WAIT_FOR_CONNECT,
    START_DETECT,
    START_RECOGNITION,
    START_ENROLL,
    START_DELETE,

} en_fsm_state;

extern en_fsm_state g_state;

extern int g_is_enrolling;
extern int g_is_deleting;

#endif
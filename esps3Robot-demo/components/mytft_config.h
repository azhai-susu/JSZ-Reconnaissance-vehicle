/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#include "../myBoard.h"


#ifdef JSZ_ROBOT_medium
#define TFT_PIN_DC 16
#define TFT_PIN_RST 42
#define TFT_PIN_MOSI 7
#define TFT_PIN_CLK 15 
#define TFT_SPI_MODE 3
#define USE_HORIZONTAL 1
#define LV_HOR_RES_MAX (240)
#define LV_VER_RES_MAX (240)
#endif

#ifdef JSZ_ROBOT_mini
#define TFT_PIN_DC 16
#define TFT_PIN_RST 17
#define TFT_PIN_MOSI 15
#define TFT_PIN_CLK 7 
#define TFT_SPI_MODE 3
#define USE_HORIZONTAL 2
#define LV_HOR_RES_MAX (240)
#define LV_VER_RES_MAX (135)
#endif

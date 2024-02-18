
#ifndef _BSP_BOARD_H_
#define _BSP_BOARD_H_

#include "driver/gpio.h"

#define FUNC_LCD_EN     (1)

#define LCD_4r3_480x272    (1)   // 4.3 inch

#if LCD_4r3_480x272
#define LCD_WIDTH       (480)
#define LCD_HEIGHT      (272)
#endif
    // 
#define GPIO_LCD_BL     (GPIO_NUM_NC)
#define GPIO_LCD_RST    (GPIO_NUM_NC)

#define GPIO_LCD_DE     (GPIO_NUM_17)
#define GPIO_LCD_VSYNC  (GPIO_NUM_18)
#define GPIO_LCD_HSYNC  (GPIO_NUM_8)
#define GPIO_LCD_PCLK   (GPIO_NUM_3)

#define GPIO_LCD_R0    (GPIO_NUM_41)
#define GPIO_LCD_R1    (GPIO_NUM_40)
#define GPIO_LCD_R2    (GPIO_NUM_39)
#define GPIO_LCD_R3    (GPIO_NUM_38)
#define GPIO_LCD_R4    (GPIO_NUM_0)

#define GPIO_LCD_G0    (GPIO_NUM_45)
#define GPIO_LCD_G1    (GPIO_NUM_48)
#define GPIO_LCD_G2    (GPIO_NUM_47)
#define GPIO_LCD_G3    (GPIO_NUM_21)
#define GPIO_LCD_G4    (GPIO_NUM_14)
#define GPIO_LCD_G5    (GPIO_NUM_13)

#define GPIO_LCD_B0    (GPIO_NUM_12)
#define GPIO_LCD_B1    (GPIO_NUM_11)
#define GPIO_LCD_B2    (GPIO_NUM_10)
#define GPIO_LCD_B3    (GPIO_NUM_9)
#define GPIO_LCD_B4    (GPIO_NUM_46)


#define FUNC_I2C_EN     (1)
#define GPIO_I2C_SCL    (15)
#define GPIO_I2C_SDA    (16)


#define SDMMC_BUS_WIDTH (1)
#define GPIO_SDMMC_CLK  (GPIO_NUM_NC)
#define GPIO_SDMMC_CMD  (GPIO_NUM_NC)
#define GPIO_SDMMC_D0   (GPIO_NUM_NC)
#define GPIO_SDMMC_D1   (GPIO_NUM_NC)
#define GPIO_SDMMC_D2   (GPIO_NUM_NC)
#define GPIO_SDMMC_D3   (GPIO_NUM_NC)
#define GPIO_SDMMC_DET  (GPIO_NUM_NC)

#endif 

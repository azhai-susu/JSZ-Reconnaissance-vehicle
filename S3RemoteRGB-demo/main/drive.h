/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#ifndef DRIVE_H
#define DRIVE_H

#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/i2s.h"

#define WIFI_SSID "ESP-72DB0"   //遥控器热点名称
#define WIFI_PASS "ESPS3-72DB0" //遥控器热点密码

// ADC
#define ADC_PIN_LY 4
#define ADC_PIN_LX 5
#define ADC_PIN_BUTTON 6
#define ADC_PIN_BAT 7
#define ADC_CHG_LY ADC_CHANNEL_3
#define ADC_CHG_LX ADC_CHANNEL_4
#define ADC_CHG_BUTTON ADC_CHANNEL_5
#define ADC_CHG_BAT ADC_CHANNEL_6

// AUDIO
#define I2S_NUM 1
#define IIS_SCLK 44
#define IIS_LCLK 43
#define IIS_DSIN 42
#define IIS_DOUT -1

// SD CARD
#define MOUNT_POINT "/sdcard"
#define PIN_NUM_MISO 1
#define PIN_NUM_MOSI 2
#define PIN_NUM_CLK 19
#define PIN_NUM_CS 20

struct remote_information
{
    char wifi_ssid[30];
    char wifi_pass[30];

    char name[30];
    char password[30];
    char localIp[20];
    char remoteIp[20];
    char wifi_ap_ssid[16];
    char wifi_ap_pass[16];

    uint8_t msg_Tbuff_len; //所发送信息的长度

    uint8_t usageMode;    //使用模式，0-待机，1-扫描，2-连接，3-控制
    uint16_t localPort;
    uint16_t remotePort;
    uint32_t sendMsgId;   //发送消息ID
    uint32_t receiveMsgId;//接收消息ID

    uint8_t *sendBuff;    //发送数据缓存
    uint8_t *sendBuff_msg;//msg用发送小缓存

    int16_t temperature; //温度
    int16_t electricityQuantity; //电量
};

extern struct remote_information esps3_remote;


extern int handle_lx;
extern int handle_ly;
extern int handle_rx;
extern int handle_ry;
extern int charge_voltage;
extern int bat_voltage;
extern uint8_t button_brightness;
extern uint8_t button_focus;
extern uint8_t button_photograph;
extern uint8_t button_arm1;
extern uint8_t button_arm2;
extern uint8_t button_back;
extern uint8_t button_set;

extern uint8_t Wifi_is_ap;

extern int SpeedL;
extern int SpeedR;
extern int Brightness;

extern uint8_t audio_volume;

extern uint8_t screen_now;

void drive_init(void);
void get_keyboard(void);
void audio_play(uint8_t *adata, uint32_t datasize);


#endif




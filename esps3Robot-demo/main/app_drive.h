/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#ifndef APP_DRIVE_H
#define APP_DRIVE_H

#include "driver/gpio.h"
#include "nvs_flash.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include <esp_heap_caps.h>
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include<string.h>
#include "../myBoard.h"

#define STORAGE_NAMESPACE "robot_info"
#define LAST_INFO_KEY "last_info"
#define LAST_INFO_KEY2 "last_info2"

#define EXAMPLE_ESP_MAXIMUM_RETRY  20 //WIFI最大重连次数
#define DEFAULT_ESP_WIFI_SSID      "ESP-72DB0"
#define DEFAULT_ESP_WIFI_PASS      "ESPS3-72DB0"
#define DEFAULT_ESP_WIFI_AP_SSID      " "
#define DEFAULT_ESP_WIFI_AP_PASS      " "

#define DEFAULT_ESP_NAME           "JSZ-ROBOT"
#define DEFAULT_ESP_PASS           "1234"
#define SOCK_PORT                  10000

#ifdef JSZ_ROBOT_mini
    //MOTOR
    #define MOTOR_L 1
    #define MOTOR_R 2
    //LEDC
    #define ledc_timer_camera LEDC_TIMER_0
    #define ledc_timer_motor_led LEDC_TIMER_1
    #define ledc_timer_servo LEDC_TIMER_2

    #define ledc_channel_camera LEDC_CHANNEL_0
    #define ledc_channel_motor_led LEDC_CHANNEL_1
    #define ledc_channel_motor_l1 LEDC_CHANNEL_2
    #define ledc_channel_motor_r1 LEDC_CHANNEL_3
    //#define ledc_channel_motor_l2 LEDC_CHANNEL_4
    //#define ledc_channel_motor_r2 LEDC_CHANNEL_5

    #define ledc_channel_servo4 -1
    #define ledc_channel_servo3 LEDC_CHANNEL_4
    #define ledc_channel_servo2 LEDC_CHANNEL_5
    #define ledc_channel_servo1 LEDC_CHANNEL_6

    #define ledc_freq_timer0 10000000
    #define ledc_freq_timer1 10000
    #define ledc_freq_timer2 50

    #define ledc_pin_camera_xclk 36
    #define ledc_pin_motor_l1a 5
    #define ledc_pin_motor_l1b 6
    #define ledc_pin_motor_r1a 8
    #define ledc_pin_motor_r1b 18
    #define ledc_pin_led 19
    //#define ledc_pin_servo1 -1
    #define ledc_pin_servo2 10
    //#define ledc_pin_servo3 10

    //ADC
    #define ADC_CHG_BAT ADC1_CHANNEL_3

    //ARMS
    #define out_pin_arms 9
    #define out_pin_armsb 46

    //AUDIO
    #define AUDIO_I2S_PORT I2S_NUM_1
    #define AUDIO_CLK_PIN 12 
    #define AUDIO_WS_PIN 13
    #define AUDIO_DATA_PIN 11

    //SPK
    #define SPK_DATA_PIN 44


    //KEYBOARD
    #define KEY_PIN_SET 0

    //WS2812
    #define WS2812_PIN -1 
#endif

#ifdef JSZ_ROBOT_medium
    //MOTOR
    #define MOTOR_L 1
    #define MOTOR_R 2
    //LEDC
    #define ledc_timer_camera LEDC_TIMER_0
    #define ledc_timer_motor_led LEDC_TIMER_1
    #define ledc_timer_servo LEDC_TIMER_2

    #define ledc_channel_camera LEDC_CHANNEL_0
    #define ledc_channel_motor_led LEDC_CHANNEL_1
    #define ledc_channel_motor_l1 LEDC_CHANNEL_2
    #define ledc_channel_motor_r1 LEDC_CHANNEL_3
    //#define ledc_channel_motor_l2 LEDC_CHANNEL_4
    //#define ledc_channel_motor_r2 LEDC_CHANNEL_5

    #define ledc_channel_servo4 -1
    #define ledc_channel_servo3 LEDC_CHANNEL_4
    #define ledc_channel_servo2 LEDC_CHANNEL_5
    #define ledc_channel_servo1 LEDC_CHANNEL_6

    #define ledc_freq_timer0 10000000
    #define ledc_freq_timer1 10000
    #define ledc_freq_timer2 50

    #define ledc_pin_camera_xclk 14
    #define ledc_pin_motor_l1a 1
    #define ledc_pin_motor_l1b 2
    #define ledc_pin_motor_r1a 40
    #define ledc_pin_motor_r1b 41
    #define ledc_pin_led 46
    #define ledc_pin_servo1 36
    #define ledc_pin_servo2 37
    //#define ledc_pin_servo3 -1

    //ADC
    #define ADC_CHG_BAT ADC1_CHANNEL_3

    //ARMS
    #define out_pin_arms 44
    #define out_pin_armsb 3

    //AUDIO
    #define AUDIO_I2S_PORT I2S_NUM_1
    #define AUDIO_CLK_PIN 5 
    #define AUDIO_WS_PIN 6
    #define AUDIO_DATA_PIN 4

    //SPK
    #define SPK_DATA_PIN 45


    //KEYBOARD
    #define KEY_PIN_SET 0

    //WS2812
    #define WS2812_PIN 35 
#endif


struct robot_information
{
    char wifi_ssid[30];
    char wifi_pass[30];

    char name[30];
    char password[30];
    char localIp[20];
    char remoteIp[20];

    uint8_t msg_Tbuff_len; //所发送信息的长度

    uint8_t usageMode;    //使用模式，0-待机，，1-遥控器A，2-遥控器B，3-安卓APP，4-IOS APP，5-其它
    uint16_t localPort;
    uint16_t remotePort;
    uint32_t sendMsgId;   //发送消息ID
    uint32_t receiveMsgId;//接收消息ID

    uint8_t *sendBuff;    //发送数据缓存
    uint8_t *sendBuff_msg;//msg用发送小缓存

    //摄像头相关
    uint8_t CameraState; //摄像头状态，0未初始化或初始化失败，1硬件初始化OK，2软件初始化OK
    uint8_t CameraEnable; //摄像头是否使能，0-不使能，1-使能，2-过度
    uint8_t ImgSize; //图像尺寸，枚举变量
    uint8_t ImgQuality; //图像质量，0-63，值越小质量越高
    uint8_t Vflip; //上下翻转
    uint8_t Hflip; //左右翻转
    uint8_t focus; //聚焦，0-关闭，1-单次，2-连续
    uint8_t audioEnable; //是否使能音频
    uint8_t volume; //音量
    uint8_t getImgTo;//获取图像缓存指针

    uint16_t frameRate; //帧率*10
    int16_t temperature; //温度
    int16_t electricityQuantity; //电量
};

extern struct robot_information esps3_robot;
extern uint16_t time_r; //用于失联重启计数

extern void* udpSendBuff;
extern void* audioBuff1;
extern void* audioBuff2;

extern uint8_t audioIdleBuff; //空闲缓存，这次写入的目标
extern uint8_t audioWritingBuff; //目前正在写入的缓存
extern size_t audioDataSize; //音频发送长度
extern size_t audioBuff1Size; //缓存1当前数据量
extern size_t audioBuff2Size; //缓存2当前数据量
extern uint8_t* pBuff; //当前存储指针定位

extern uint8_t reverse_ml1;
extern uint8_t reverse_ml2;
extern uint8_t reverse_mr1;
extern uint8_t reverse_mr2;
extern uint8_t reverse_s1;
extern uint8_t reverse_s2;
extern uint8_t reverse_s3;
extern uint8_t reverse_s4;

extern uint8_t led_state;//0-关闭，1-常亮，2-熄灭，3-慢闪，4-快闪

extern float angle_x;
extern float angle_y;

extern uint8_t audio_ready;


esp_err_t save_robot_info(char *key_);
esp_err_t read_robot_info(char *key_);

void robotDriveInit(void);
void headlampSet(int bri);
void servoSet(uint8_t ch,float angle);
void motorSet(uint8_t motor, int spe);
void heap_show(void);
void robotStop(void);
void audioInit(void);
size_t writeAudioDataToBuff(void);
uint8_t* getAudioDataFromBuff(void);
int get_bat_voltage(void);
void play_i2s_init(void);
void all_i2s_deinit(void);

#endif
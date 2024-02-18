esps3Robot-demo 的编译环境为IDF4.4

IDF下载地址：https://dl.espressif.cn/dl/esp-idf/?idf=4.4

1.  CPU选择ESP32S3-N4R2

2.  esps3Robot-demo\myBoard.h 中通过两个宏定义选择主板型号
    //#define JSZ_ROBOT_mini 1//使用迷你机器人主板（无喇叭和彩灯）
    #define JSZ_ROBOT_medium 1//使用3D打印款中号机器人主板

3.  esps3Robot-demo\main\app_camera.h 文件定义摄像头IO
    esps3Robot-demo\components\mytft_config.h 文件定义显示屏IO
    esps3Robot-demo\main\app_drive.h 文件定义其它外设

4.  esps3Robot-demo\main\app_drive.h 里 
    "DEFAULT_ESP_WIFI_SSID" 与 "DEFAULT_ESP_WIFI_PASS" 定义WiFi名和密码，需要与遥控器一致

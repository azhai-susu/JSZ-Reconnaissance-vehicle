S3RemoteRGB-demo 的编译环境为IDF5.1

IDF下载地址：https://dl.espressif.cn/dl/esp-idf/?idf=4.4

1.  CPU选择ESP32S3-N8R8

2.  下载程序和调试时需要拔除SD卡，部分IO与USB口共用，下载程序使用模块自带的USB串口

3.  请在 S3RemoteRGB-demo\main\drive.h 文件中设置与机器人一致的WiFi名和密码
    #define WIFI_SSID "ESP-72DB0"   //遥控器热点名称
    #define WIFI_PASS "ESPS3-72DB0" //遥控器热点密码

4.  遥控器所有按键是共用一个IO的，采用电阻分压 ADC采集的方式区分按键，但由于ESP32的ADC不太准确，
    所以可能不同的板子会识别错误，大家要自行调试更改按键值（S3RemoteRGB-demo\main\drive.c）
    uint16_t ADC_BTVAL_A = 3383;
    uint16_t ADC_BTVAL_B = 2695;
    uint16_t ADC_BTVAL_C = 1999;
    uint16_t ADC_BTVAL_D = 1617;
    uint16_t ADC_BTVAL_E = 490;
    uint16_t ADC_BTVAL_F = 1261;
    uint16_t ADC_BTVAL_AB = 2347;
    uint16_t ADC_BTVAL_AC = 1801;
    uint16_t ADC_BTVAL_AD = 1482;

5.  在 S3RemoteRGB-demo\main\drive.c 文件 get_keyboard 函数调试ADC按键

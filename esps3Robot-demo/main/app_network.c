/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "app_camera.h"

#include "app_network.h"
#include "mUdpTransmission.h"
#include "app_drive.h"
#include "ov5640_focus.h"
#include "driver/i2s.h"
#include "app_led.h"

#define PORT 10000//CONFIG_EXAMPLE_PORT
int sock;
struct sockaddr_storage source_addr;//创建一个结构体用于保存发送方IP地址

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif


/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;
char msg_Tbuff[256]; //发送给接收到的其它数据信息
uint8_t retransmission = 0; //是否重发部分数据
uint8_t paketState[60]={0xff};//数据包接收状态
uint8_t net_connected = 0; //连接状态

extern camera_fb_t *fb;

static void udp_server_task(void *pvParameters);


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		//WiFi驱动程序已启动
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		//WiFi未连接或掉线
        net_connected = 0;
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) { //重连次数
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            esp_restart();//重启CPU
        }
        ESP_LOGI(TAG,"connect to the AP fail");
        esps3_robot.localIp[0] = 0;
	    esps3_robot.remoteIp[0] = 0;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		//WiFi连接成功，获取到IP
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(esps3_robot.localIp,IPSTR,IP2STR(&event->ip_info.ip));//IPSTR="%d.%d.%d.%d"
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        net_connected = 1;
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());//初始化TCP/IP堆栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());//创建默认事件循环
    esp_netif_create_default_wifi_sta();//创建默认WIFI STA，如果出现任何初始化错误，此API将中止

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));//初始化WiFi

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",	
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };
    memcpy(wifi_config.sta.ssid,esps3_robot.wifi_ssid,strlen(esps3_robot.wifi_ssid));
	memcpy(wifi_config.sta.password,esps3_robot.wifi_pass,strlen(esps3_robot.wifi_pass));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /*等待连接建立（WIFI_CONNECTED_BIT）或连接失败达到最大重试次数（WIFI_FAIL_BIT）。这些位由event_handler（）设置（见上文）*/
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /*判断WiFi状态*/
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 esps3_robot.wifi_ssid, esps3_robot.wifi_pass);
		//启动UDP线程
		xTaskCreatePinnedToCore(&udp_server_task, "udp_server", 1024 * 6, (void*)AF_INET, 6, NULL, 1);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "Failed to connect to SSID:%s, password:%s",
                 esps3_robot.wifi_ssid, esps3_robot.wifi_pass);
    } else {
        ESP_LOGW(TAG, "UNEXPECTED EVENT");
    }
}

extern uint8_t led_state;//0-关闭，1-常亮，2-熄灭，3-慢闪，4-快闪
//UDP发送线程
extern int cam_verify_jpeg_soi(const uint8_t *inbuf, uint32_t length);
static void udp_send_task(void *pvParameters)
{
    uint8_t CameraEnable = 1;
	int64_t last_frame = 0;
    int64_t frame_time;

    esps3_robot.msg_Tbuff_len = 0;
    last_frame = esp_timer_get_time();
    led_state = 3;
    while(1)
    {
        //开启/关闭摄像头
		if(CameraEnable != esps3_robot.CameraEnable)
		{
			CameraEnable = esps3_robot.CameraEnable;
			if((CameraEnable == 1) && (esps3_robot.CameraState == 2))//启动摄像头
			{
				ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel_camera, 1);
            	ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel_camera);
                vTaskDelay(5);
                uint8_t QualityMax = 0;
                sensor_t *s = esp_camera_sensor_get();
                //限制图像尺寸
                if(esps3_robot.ImgSize>=13){
                    QualityMax = 15;
                }else if(esps3_robot.ImgSize >= 11){
                    QualityMax = 15;
                }else if(esps3_robot.ImgSize >= 10){
                    QualityMax = 5;
                }
                if(esps3_robot.ImgQuality < QualityMax)
                    esps3_robot.ImgQuality = QualityMax;
                //设置摄像头参数
                s->set_quality(s, esps3_robot.ImgQuality);
                s->set_framesize(s, (framesize_t)esps3_robot.ImgSize);
                s->set_vflip(s, esps3_robot.Vflip); //水平翻转
                s->set_hmirror(s, esps3_robot.Hflip); //垂直翻转
			}
			else if(CameraEnable == 0) //关闭摄像头
			{
				ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel_camera, 0);
            	ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel_camera);
			}
		}
		//采集图像
		if((esps3_robot.CameraEnable == 1) && (esps3_robot.CameraState == 2))
		{
            esp_camera_fb_return(fb);
			fb = esp_camera_fb_get();
			if (fb == NULL)
			{
				vTaskDelay(100);
				ESP_LOGE(TAG, "Get image failed!");
                esps3_robot.getImgTo = 0;
			}
			else
			{
                uint16_t len1 = 10240;
                if(len1 > fb->len)
                    len1 = fb->len;
                int len2 = cam_verify_jpeg_soi((uint8_t *)fb->buf, len1);//查找文件头
                if(len2 >= 0)
                {
                    if(len2 > 0)
                        printf("len = %d\r\n",len2);
                    fb->buf += len2;
                    fb->len -= len2;
                    //ESP_LOGI("esp", "IMG-LEN:  %u", fb->len);
                    esps3_robot.getImgTo = 1;
                    //计算帧率
                    int64_t fr_end = esp_timer_get_time();
                    frame_time = (fr_end - last_frame) / 1000;
                    last_frame = fr_end;
                    esps3_robot.frameRate = 10000 / (uint32_t)frame_time;
                    //ESP_LOGI("esp", "MJPG:  %ums (%.1ffps)", (uint32_t)frame_time, esps3_robot.frameRate/10.0);
                    if(esps3_robot.audioEnable && (frame_time > 64))
                        writeAudioDataToBuff();//获取音频数据
                }
			}
		}
        else
        {
            size_t writeSize;
            //i2s_read(AUDIO_I2S_PORT, (uint8_t *)(audioBuff1+1500), 5120, &writeSize, 0);//及时清空音频数据
        }
        //发送数据
        if(esps3_robot.usageMode) //设备已连接
        {
            if(retransmission)//有数据需要重发
            {
                retransmission = 0;
                uint16_t packet_count = getPacketCount();               
                uint16_t i;
                for(i = 0; i < packet_count; i ++){
                    if((paketState[i/8] & (0x80>>(i%8))) == 0){
                        udpSendBuffDataPacket((uint8_t*)udpSendBuff, sock, source_addr, i);
                        paketState[i/8] |= (0x80>>(i%8));
                        //printf("Send packet:%d\r\n",i);
                    }
                }
            }
            else if(esps3_robot.getImgTo)//有图像数据发送
            {
                uint8_t* aBuff = getAudioDataFromBuff();//获取音频缓存
                if(esps3_robot.msg_Tbuff_len == 0)
                    esps3_robot.msg_Tbuff_len = sprintf(msg_Tbuff,"{\"RobotIp\":\"%s\",\"CoreTemperature\":\"%d\",\"electricityQuantity\":\"%d\"}", esps3_robot.localIp,esps3_robot.temperature,esps3_robot.electricityQuantity);
                //printf("audioDataSize:%d\r\n",audioDataSize);
                //for(int i = 0; i < audioDataSize; i++)
                //    *(aBuff+i) = i % 100;
                dataJoin((uint8_t*)udpSendBuff, ++esps3_robot.sendMsgId, (uint8_t *)fb->buf, fb->len, aBuff, audioDataSize, (uint8_t*)msg_Tbuff, esps3_robot.msg_Tbuff_len);
                udpSendBuffData((uint8_t*)udpSendBuff, sock, source_addr);
                esps3_robot.msg_Tbuff_len = 0;
                esps3_robot.getImgTo = 0;
            }
            else if(esps3_robot.msg_Tbuff_len)
            {
                dataJoin((uint8_t*)udpSendBuff, ++esps3_robot.sendMsgId, (uint8_t *)"", 0, (uint8_t*)"", 0, (uint8_t*)msg_Tbuff, esps3_robot.msg_Tbuff_len);
                udpSendBuffData((uint8_t*)udpSendBuff, sock, source_addr);
                esps3_robot.msg_Tbuff_len = 0;
            }
        }
        else if(esps3_robot.msg_Tbuff_len)
        {
            dataJoin((uint8_t*)udpSendBuff, ++esps3_robot.sendMsgId, (uint8_t *)"", 0, (uint8_t*)"", 0, (uint8_t*)msg_Tbuff, esps3_robot.msg_Tbuff_len);
            udpSendBuffData((uint8_t*)udpSendBuff, sock, source_addr);
            msg_Tbuff[esps3_robot.msg_Tbuff_len] = 0;
            esps3_robot.msg_Tbuff_len = 0;
        }
        vTaskDelay(5);
    }
}

//UDP线程
static void udp_server_task(void *pvParameters)
{
    char rx_buffer[256]; //udp接收缓存
    char addr_str[30];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;
    int servo_temp = 90;
    
    esps3_robot.usageMode = 0;
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(PORT);
    ip_protocol = IPPROTO_IP;
    //创建socket
    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        esp_restart();
    }
    //绑定socket
    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        esp_restart();
    }
    socklen_t socklen = sizeof(source_addr);
    //启动UDP发送线程
    xTaskCreatePinnedToCore(&udp_send_task, "udp_send", 1024 * 4, (void*)AF_INET, 5, NULL, 1);
    while (1) {
        //阻塞接收数据
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
        }
        else {
            //获取发送方地址
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            //输出接收内容
            rx_buffer[len] = 0; //接收完成后在缓存末尾赋0，如果是字符串即添加结束符
            //ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
            //解析接收到的数据
            uint32_t msg_id;
            uint8_t msg_type;
            char* dataBuff = (char*)parsingPackets((uint8_t *)rx_buffer, len, &msg_id, &msg_type);
            if(dataBuff != (char*)NULL)//校验通过
            {   
                //ESP_LOGI(TAG, "%s", (char*)dataBuff);
                if(msg_type == 1)//普通消息
                {
                    cJSON *pJsonRoot = cJSON_Parse((char*)dataBuff);
                    if(pJsonRoot == NULL)
                    {
                        cJSON_Delete(pJsonRoot);//释放内存
                        continue;
                    }
                    cJSON *root = cJSON_GetObjectItem(pJsonRoot, "Pass");                       
                    if(strcmp(root->valuestring, esps3_robot.password) == 0)//密码正确
                    {
                        time_r = 0;
                        root = cJSON_GetObjectItem(pJsonRoot, "Act");
                        if(root == NULL)
                        {
                            cJSON_Delete(pJsonRoot);//释放内存
                            continue;
                        }
                        if(strcmp(root->valuestring, "Connect") == 0)//连接请求
                        {
                            esps3_robot.usageMode = 1;
                            esps3_robot.CameraEnable = 2;
                            sprintf(esps3_robot.remoteIp,addr_str);

                            root = cJSON_GetObjectItem(pJsonRoot, "ImgSize");
                            if(root != NULL)
                                esps3_robot.ImgSize = root->valueint;
                            root = cJSON_GetObjectItem(pJsonRoot, "ImgQuality");
                            if(root != NULL)
                                esps3_robot.ImgQuality = root->valueint;
                            root = cJSON_GetObjectItem(pJsonRoot, "Vflip");
                            if(root != NULL)
                                esps3_robot.Vflip = root->valueint;
                            root = cJSON_GetObjectItem(pJsonRoot, "Hflip");
                            if(root != NULL)
                                esps3_robot.Hflip = root->valueint;
                            root = cJSON_GetObjectItem(pJsonRoot, "Mic");
                            if(root != NULL)
                                esps3_robot.audioEnable = root->valueint;
                            root = cJSON_GetObjectItem(pJsonRoot, "AGain");
                            if(root != NULL)
                                esps3_robot.volume = root->valueint;
                            root = cJSON_GetObjectItem(pJsonRoot, "Reverse");
                            if(root != NULL)
                            {
                                reverse_ml1 = 0;
                                reverse_ml2 = 0;
                                reverse_mr1 = 0;
                                reverse_mr2 = 0;
                                reverse_s1 = 0;
                                reverse_s2 = 0;
                                reverse_s3 = 0;
                                reverse_s4 = 0;
                                uint8_t rev = root->valueint;
                                if((rev << 0) & 0x80)
                                    reverse_ml1 = 1;
                                if((rev << 1) & 0x80)
                                    reverse_ml2 = 1;
                                if((rev << 2) & 0x80)
                                    reverse_mr1 = 1;
                                if((rev << 3) & 0x80)
                                    reverse_mr2 = 1;
                                if((rev << 4) & 0x80)
                                    reverse_s1 = 1;
                                if((rev << 5) & 0x80)
                                    reverse_s2 = 1;
                                if((rev << 6) & 0x80)
                                    reverse_s3 = 1;
                                if((rev << 7) & 0x80)
                                    reverse_s4 = 1;
                            }

                            ESP_LOGI(TAG, "Remote address:%s\r\n",esps3_robot.remoteIp);
                            esps3_robot.msg_Tbuff_len = sprintf(msg_Tbuff,"{\"RobotIp\":\"%s\",\"Act\":\"Connected\"}", esps3_robot.localIp);
                            ESP_LOGI(TAG, "Connection successful!");
                            save_robot_info(LAST_INFO_KEY);//保存参数
                            save_robot_info(LAST_INFO_KEY2);//保存参数
                        }
                        else if(strcmp(root->valuestring, "close") == 0)//关闭请求
                        {
                            esps3_robot.usageMode = 0;
                            esps3_robot.CameraEnable = 0;
                            robotStop();  
                            ESP_LOGI(TAG, "Disconnect!");
                        }
                        else if(esps3_robot.usageMode > 0)//控制信息
                        {
                            if(esps3_robot.CameraEnable == 2)
                                esps3_robot.CameraEnable = 1;

                            root = cJSON_GetObjectItem(pJsonRoot, "SL");
                            if(root != NULL)
                                motorSet(MOTOR_L, root->valueint);
                            root = cJSON_GetObjectItem(pJsonRoot, "SR");
                            if(root != NULL)
                                motorSet(MOTOR_R, root->valueint);
                            root = cJSON_GetObjectItem(pJsonRoot, "Bri");
                            if(root != NULL){
                                headlampSet(root->valueint);
                                #ifdef JSZ_ROBOT_medium
                                app_led_set_all(root->valueint, root->valueint, root->valueint);
                                #endif
                            }
                            root = cJSON_GetObjectItem(pJsonRoot, "Ag");
                            if(root != NULL)
                            {
                                servoSet(ledc_channel_servo2,root->valueint);
                            }
                            root = cJSON_GetObjectItem(pJsonRoot, "Foc");
                            if(root != NULL)
                            {

                            }

                            root = cJSON_GetObjectItem(pJsonRoot, "MRX");
                            if(root != NULL)
                            {
                                angle_x += root->valueint / 10;
                                if(angle_x > 180)
                                    angle_x = 180;
                                if(angle_x < 0)
                                    angle_x = 0;
                                servoSet(ledc_channel_servo1,angle_x);
                            }
                            root = cJSON_GetObjectItem(pJsonRoot, "MRY");
                            if(root != NULL)
                            {
                                angle_y += root->valueint / 10;
                                if(angle_y > 180)
                                    angle_y = 180;
                                if(angle_y < 0)
                                    angle_y = 0;
                                servoSet(ledc_channel_servo2,angle_y);
                            }

                            root = cJSON_GetObjectItem(pJsonRoot, "Arms");
                            if(root != NULL)
                                gpio_set_level(out_pin_arms, root->valueint);
                            root = cJSON_GetObjectItem(pJsonRoot, "ArmsB");
                            if(root != NULL)
                                gpio_set_level(out_pin_armsb, root->valueint);
                        }
                        else
                        {
                            esps3_robot.usageMode = 1;
                            esps3_robot.CameraEnable = 2;
                            sprintf(esps3_robot.remoteIp,addr_str);
                        }
                    }
                    else
                    {
                        ESP_LOGW(TAG, "Password error!");
                    }
                    cJSON_Delete(pJsonRoot);//释放内存
                }else if(msg_type == 2){//回应消息
                    uint32_t frame_id = getFrameId();
                    if(msg_id == frame_id){
                        uint16_t packetCount = getPacketCount();
                        uint16_t reQuantity = 0;
                        for(uint16_t i = 0; i < packetCount; i ++){
                            if((dataBuff[i/8] & (0x80 >> (i % 8))) == 0){
                                reQuantity ++;
                            }  
                        }
                        if(reQuantity){
                            uint8_t len = packetCount / 8;
                            if(packetCount % 8 > 0)
                                len++;
                            if(len > 60)
                                len = 60;
                            for(uint8_t i = 0; i < len; i++){
                                paketState[i] = dataBuff[i];
                            }
                            retransmission = 1;
                            //printf("reQuantity = %u\r\n",reQuantity);
                        }else{
                            //printf("Sent successfully!\r\n");
                            retransmission = 0;
                        }
                    
                    }
                }else if(msg_type == 3){//设备搜索
                    ESP_LOGI(TAG, "Get device:%s", addr_str);
                    if(esps3_robot.usageMode == 0)//空闲状态
                        esps3_robot.msg_Tbuff_len = sprintf(msg_Tbuff,"{\"name\":\"%s\",\"state\":\"%s\"}", esps3_robot.name,"idle");
                    else
                        esps3_robot.msg_Tbuff_len = sprintf(msg_Tbuff,"{\"name\":\"%s\",\"state\":\"%s\"}", esps3_robot.name,"busy");
                }
            }
        }
    }
}


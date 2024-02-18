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
#include <esp_heap_caps.h>
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "func.h"
#include "mUdpTransmission.h"
#include "drive.h"
#include "JPEGDecoder/JPEGDecoder.h"
#include "disp_driver.h"




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


extern uint8_t file_save(uint8_t *buf, size_t len);
char addr_str[30];

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


static const char *TAG = "FUNC";
#define PORT 10000//CONFIG_EXAMPLE_PORT
int sock;
struct sockaddr_storage source_addr;//创建一个结构体用于保存发送方IP地址
static struct sockaddr_in dest_addr_find;

char dropdown_buff[30][36] = {0};
uint8_t picture_buff[2][16000]={0};
uint8_t lcdCanvas[2][16000];
uint8_t audio_buff[10240];

static int s_retry_num = 0;
char msg_Tbuff[256]; //接收到的其它数据信息
uint8_t retransmission = 0; //是否重发部分数据
uint8_t paketState[60]={0xff};//数据包接收状态

int jpg_len[2] = {0};

char bat1[5] = {0};
char bat2[5] = {0};
int bat2_2 = 0;
int fps = 0;
int fps_n = 0;
uint8_t ph_time = 0;

static void udp_server_task(void *pvParameters);

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))
//输出JPG信息
void jpegInfo() {
  ESP_LOGI(TAG,"===============\n");
  ESP_LOGI(TAG,"JPEG image info\n");
  ESP_LOGI(TAG,"===============\n");
  ESP_LOGI(TAG,"Width      :%d\n",JPEGDecoder_width);
  ESP_LOGI(TAG,"Height     :%d\n",JPEGDecoder_height);
  ESP_LOGI(TAG,"Components :%d\n",JPEGDecoder_comps);
  ESP_LOGI(TAG,"MCU / row  :%d\n",JPEGDecoder_MCUSPerRow);
  ESP_LOGI(TAG,"MCU / col  :%d\n",JPEGDecoder_MCUSPerCol);
  ESP_LOGI(TAG,"Scan type  :%d\n",JPEGDecoder_scanType);
  ESP_LOGI(TAG,"MCU width  :%d\n",JPEGDecoder_MCUWidth);
  ESP_LOGI(TAG,"MCU height :%d\n",JPEGDecoder_MCUHeight);
  ESP_LOGI(TAG,"===============\n");
}
//jpg图片解码显示到屏幕，参数为起始坐标
lv_disp_drv_t lvdrv_;
lv_area_t area;
void renderJPEG(int xpos, int ypos) {

  //获取图像尺寸和块尺寸
  uint16_t *pImg;
  uint16_t mcu_w = JPEGDecoder_MCUWidth; //块宽度
  uint16_t mcu_h = JPEGDecoder_MCUHeight; //块高度
  uint32_t max_x = JPEGDecoder_width; //图像宽度
  uint32_t max_y = JPEGDecoder_height; //图像高度
  
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w); //最右边不完整块宽度
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h); //最下边不完整块高度

  //当前MCU尺寸
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // 遍历读取图像块
  while (JPEGDecoder_read()) {
    
    // 当前图像块指针
    pImg = JPEGDecoder_pImage;

    // 计算出在屏幕的显示位置
    int mcu_x = JPEGDecoder_MCUx * mcu_w;
    int mcu_y = JPEGDecoder_MCUy * mcu_h;

    //当前MCU尺寸
    if (mcu_x + mcu_w <= max_x)
        win_w = mcu_w;
    else
        win_w = min_w;
    if(mcu_y + mcu_h <= max_y)
        win_h = mcu_h;
    else
        win_h = min_h;

    //将图像块保存到屏幕缓存
    uint16_t y2 = (JPEGDecoder_MCUy * mcu_h) % 16;
    uint16_t *p1;
    for(uint8_t y = 0; y < win_h; y ++)
    {
        if(mcu_y + y <= LCD_HEIGHT)
        {
            p1 = (uint16_t*)&lcdCanvas[JPEGDecoder_MCUy%2] + (y * JPEGDecoder_width + mcu_x);
            for(uint8_t x = 0; x < win_w; x ++)
            {
                if(mcu_x + x <= LCD_WIDTH)
                {
                  *(p1 + x) = *pImg;
                  pImg ++;
                }
            }
        }
    }

    //缓存满或块接收完毕时将缓存刷新到屏幕
    if((JPEGDecoder_MCUx + 1) * mcu_w >= max_x)//图片解码到了最右边
    {
        esp_lcd_panel_draw_bitmap(panel_handle_2, 0, mcu_y, max_x, mcu_y + win_h, lcdCanvas[JPEGDecoder_MCUy%2]);
    }
  }
}
//JPG解码线程
void jpg_decode(void *pvParameters)
{
	while (1)
	{
        if(jpg_len[0])
        {
            if(button_photograph)
            {
                if(file_save((uint8_t*)picture_buff[0], jpg_len[0]))
                    ph_time = 3;
                button_photograph = 0;
            }
            JPEGDecoder_decodeArray(picture_buff[0], jpg_len[0]);
            renderJPEG(0, 0);
            JPEGDecoder_abort();
            jpg_len[0] = 0;
        }
        else if(jpg_len[1])
        {
            if(button_photograph)
            {
                if(file_save((uint8_t*)picture_buff[1], jpg_len[1]))
                    ph_time = 3;
                button_photograph = 0;
            }
            JPEGDecoder_decodeArray(picture_buff[1], jpg_len[1]);
            renderJPEG(0, 0);
            JPEGDecoder_abort();
            jpg_len[1] = 0;
        }
		vTaskDelay(5);
	}	
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		//WiFi驱动程序已启动
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		//WiFi未连接或掉线
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) { //重连次数
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            esp_restart();//重启CPU
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		//WiFi连接成功，获取到IP
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(esps3_remote.localIp,IPSTR,IP2STR(&event->ip_info.ip));//IPSTR="%d.%d.%d.%d"
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {// 一台设备连接AP
        xTaskCreatePinnedToCore(&udp_server_task, "udp_server", 1024 * 6, (void*)AF_INET, 6, NULL, 0);
        printf("link..\n");
        //printf("station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {// 一台设备断开AP
        //printf("station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid); 
        printf("leave..\n");
    }
}

void wifi_init_ap(void)
{
    esp_event_handler_instance_t instance_any_id = {0};  //处理ID 实例句柄
    esp_event_handler_instance_t instance_got_ip = {0};  //处理IP 实例句柄
    
    s_wifi_event_group = xEventGroupCreate();            // 创建全局事件 
    ESP_ERROR_CHECK(esp_netif_init());                   // 初始化TCP/IP堆栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());    // 创建默认事件循环
    esp_netif_create_default_wifi_ap();		         // 创建默认WIFI AP
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // WIFI设备配置
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); 			     // WIFI设备初始化

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

	// 创建 并 初始化AP对象
    wifi_config_t wifi_configAP = {
        .ap = 
        {
            .ssid           = "",
            .ssid_len       = 0,
            .channel        = 1,
            .password       = "",
            .max_connection = 4,
            .authmode       = WIFI_AUTH_WPA_WPA2_PSK,
        }
    };
    memcpy(wifi_configAP.ap.ssid,esps3_remote.wifi_ap_ssid,strlen(esps3_remote.wifi_ap_ssid));
	memcpy(wifi_configAP.ap.password,esps3_remote.wifi_ap_pass,strlen(esps3_remote.wifi_ap_pass));
    wifi_configAP.ap.ssid_len = strlen(esps3_remote.wifi_ap_ssid);
    // 设置WIFI为 AP-STA 共存模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	// 设置AP模式配置
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP , &wifi_configAP) );
    //启动WiFi
    ESP_ERROR_CHECK(esp_wifi_start() ); 
    printf("wifi_init_ap finished. %s, %s\r\n",wifi_configAP.ap.ssid, wifi_configAP.ap.password);
    vTaskDelay(1000);
}

//UDP发送线程
static void udp_send_task(void *pvParameters)
{
    char tx_buffer[256];
    int find_count = 0;
    while(1)
    {
        if(esps3_remote.usageMode == 1)//扫描
        {
            if(find_count == 0)
            {
                for(int i = 0; i < 30; i++)
                    dropdown_buff[i][0] = 0;
            }
            int len1 = sprintf(tx_buffer,"{\"action\":\"getDevice\",\"from\":\"S3-REMOTE\"}");
            printf("SEND:%s\n",tx_buffer);
            char *txbuf_addr = tx_buffer;
            len1 = generatingPackets(++esps3_remote.sendMsgId, 3, (uint8_t*)tx_buffer, len1, &txbuf_addr);

            dest_addr_find.sin_addr.s_addr = inet_addr("255.255.255.255");
            dest_addr_find.sin_family = AF_INET;
            dest_addr_find.sin_port = htons(esps3_remote.remotePort);

            int err = sendto(sock, txbuf_addr, len1, 0, (struct sockaddr *)&dest_addr_find, sizeof(dest_addr_find));
            find_count ++;
            vTaskDelay(500);
            if(find_count >= 10)
            {
                find_count = 0;
            }
        }
        else if(esps3_remote.usageMode == 2)//连接
        {
            int len1 = sprintf(tx_buffer,"{\"Ip\":\"\",\"Pass\":\"%s\",\"Act\":\"Connect\",\"ImgSize\":6,\"ImgQuality\":10,\"Vflip\":%d,\"Hflip\":%d,\"Mic\":%d,\"AGain\":%d,\"Reverse\":%d}","1234",0,1,audio_volume,audio_volume * 3,0);
            char *txbuf_addr = tx_buffer;
            len1 = generatingPackets(++esps3_remote.sendMsgId, 1, (uint8_t*)tx_buffer, len1, &txbuf_addr);

            //dest_addr_find.sin_addr.s_addr = inet_addr(lv_textarea_get_text(ui_ScreenFindTextAreaIp));
            dest_addr_find.sin_addr.s_addr = inet_addr(addr_str);
            dest_addr_find.sin_family = AF_INET;
            dest_addr_find.sin_port = htons(esps3_remote.remotePort);

            int err = sendto(sock, txbuf_addr, len1, 0, (struct sockaddr *)&dest_addr_find, sizeof(dest_addr_find));
            find_count ++;
            vTaskDelay(500);
            if(find_count >= 10)
            {
                find_count = 0;
                esps3_remote.usageMode = 0;
            }
        }
        else if(esps3_remote.usageMode == 3)//通信
        {
            int len1;
            if(button_back)//按下返回键结束
            {
                button_back = 0;
                find_count = 0;
                esps3_remote.usageMode = 0;
                len1 = sprintf(tx_buffer,"{\"Ip\":\"\",\"Pass\":\"%s\",\"Act\":\"close\",\"SL\":%d,\"SR\":%d,\"Bri\":%d,\"Ag\":%d,\"Arms\":%d,\"ArmsB\":%d,\"MRX\":%d,\"MRY\":%d,\"MLX\":%d,\"MLY\":%d,\"Foc\":%d}",
                                            "1234", 0, 0, 0, 90, 0, 0, 0, 0, 0, 0, 0);
            }
            else
            {
                len1 = sprintf(tx_buffer,"{\"Ip\":\"\",\"Pass\":\"%s\",\"Act\":\"\",\"SL\":%d,\"SR\":%d,\"Bri\":%d,\"Ag\":%d,\"Arms\":%d,\"ArmsB\":%d,\"MRX\":%d,\"MRY\":%d,\"MLX\":%d,\"MLY\":%d,\"Foc\":%d}",
                                            "1234", SpeedL, SpeedR, Brightness, 90, button_arm1, button_arm2, handle_rx, handle_ry, handle_lx, handle_ly, button_focus);
                button_focus = 0;
            }
            char *txbuf_addr = tx_buffer;
            len1 = generatingPackets(++esps3_remote.sendMsgId, 1, (uint8_t*)tx_buffer, len1, &txbuf_addr);
            int err = sendto(sock, txbuf_addr, len1, 0, (struct sockaddr *)&dest_addr_find, sizeof(dest_addr_find));
            vTaskDelay(100);
        }
        vTaskDelay(5);
    }
}

//UDP线程
static void udp_server_task(void *pvParameters)
{
    char rx_buffer[1400]; //udp接收缓存
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;
    
    esps3_remote.usageMode = 0;
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

    printf("UDP START!\r\n");

    esps3_remote.remotePort = 10000;
    //启动UDP发送线程
    xTaskCreatePinnedToCore(&udp_send_task, "udp_send", 1024 * 4, (void*)AF_INET, 5, NULL, 0);
    xTaskCreatePinnedToCore(&jpg_decode, "jpg_decode", 1024 * 10, (void*)AF_INET, 6, NULL, 1);

    esps3_remote.usageMode = 1;
    while (1) {
        //阻塞接收数据
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
        }
        else if(len > 0){
            //获取发送方地址
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            //解析接收到的数据
            int state = importData((uint8_t*)rx_buffer, len);
            if(state == 3)
            {
                if(esps3_remote.usageMode == 1)//扫描
                {
                    len = getMsgData((uint8_t*)rx_buffer);
                    rx_buffer[len] = 0;
                    //printf("%s\r\n",rx_buffer);
                    cJSON *pJsonRoot = cJSON_Parse((char*)rx_buffer);
                    if(pJsonRoot == NULL)
                    {
                        cJSON_Delete(pJsonRoot);//释放内存
                        continue;
                    }
                    cJSON *name = cJSON_GetObjectItem(pJsonRoot, "name");
                    printf("name:%s\n",name->valuestring);

                    cJSON *state = cJSON_GetObjectItem(pJsonRoot, "state");
                    printf("state:%s\n",state->valuestring);
                    printf("ip:%s\n",addr_str);

                    esps3_remote.usageMode = 2;
                    cJSON_Delete(pJsonRoot);//释放内存
                }
                else if(esps3_remote.usageMode == 2)//连接
                {
                    len = getMsgData((uint8_t*)rx_buffer);
                    rx_buffer[len] = 0;
                    //printf("RV:%s\r\n",rx_buffer);

                    cJSON *pJsonRoot = cJSON_Parse((char*)rx_buffer);
                    if(pJsonRoot == NULL)
                    {
                        cJSON_Delete(pJsonRoot);//释放内存
                        continue;
                    }
                    cJSON *act = cJSON_GetObjectItem(pJsonRoot, "Act");
                    if(act != NULL)
                    {
                        if(strcmp(act->valuestring, "Connected") == 0) //连接成功
                        {
                            printf("Connected!\n");
                            esps3_remote.usageMode = 3;
                            button_back = 0;
                            fps_n = 0;
                        }
                    }
                    cJSON_Delete(pJsonRoot);//释放内存

                    button_photograph = 0;
                }
                else if(esps3_remote.usageMode == 3)//控制
                {
                    len = getMsgData((uint8_t*)rx_buffer);
                    rx_buffer[len] = 0;
                    //printf("RV:%s\r\n",rx_buffer);
                    
                    if(audio_volume)
                    {
                        len = getAudioData(audio_buff);
                        if(len > 0)
                            audio_play(audio_buff, len);
                    }

                    if(jpg_len[0] == 0)
                    {
                        jpg_len[0] = getPictureData(picture_buff[0]);
                    }
                    else if(jpg_len[1] == 0)
                    {
                        jpg_len[1] = getPictureData(picture_buff[1]);
                    }
                    
                }
            }
        }
    }
}



/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#include "app_drive.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"


#define TAG "Drive"

//定义缓存
void* udpSendBuff = NULL;
void* audioBuff1 = NULL;
void* audioBuff2 = NULL;

uint8_t audioIdleBuff = 1; //空闲缓存，这次写入的目标
uint8_t audioWritingBuff = 0; //目前正在写入的缓存
size_t audioDataSize = 0; //音频发送长度
size_t audioBuff1Size = 0; //缓存1当前数据量
size_t audioBuff2Size = 0; //缓存2当前数据量
uint8_t* pBuff; //当前存储指针定位

uint8_t reverse_ml1 = 0;
uint8_t reverse_ml2 = 0;
uint8_t reverse_mr1 = 0;
uint8_t reverse_mr2 = 0;
uint8_t reverse_s1 = 0;
uint8_t reverse_s2 = 0;
uint8_t reverse_s3 = 0;
uint8_t reverse_s4 = 0;


uint16_t time_r = 0; //用于失联重启计数
struct robot_information esps3_robot;

float angle_x = 90;
float angle_y = 90;

uint8_t audio_ready = 0;

//ADC
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_TP_FIT
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_11
static esp_adc_cal_characteristics_t adc1_chars;
static bool adc_calibration_init(void)
{
    esp_err_t ret;
    bool cali_enable = false;

    ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "Calibration scheme not supported, skip software calibration");
    } else if (ret == ESP_ERR_INVALID_VERSION) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else if (ret == ESP_OK) {
        cali_enable = true;
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    } else {
        ESP_LOGE(TAG, "Invalid arg");
    }

    return cali_enable;
}

/*
    卸载I2S驱动
*/
void all_i2s_deinit(void)
{
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    vTaskDelay(10);
}

void play_i2s_init(void)
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 36000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LOWMED,
        .dma_buf_count = 2,
        .dma_buf_len = 300,
        .bits_per_chan = I2S_BITS_PER_SAMPLE_16BIT};
    i2s_pin_config_t pin_config = {
        .mck_io_num = -1,
        .bck_io_num = AUDIO_CLK_PIN,   // IIS_SCLK
        .ws_io_num = AUDIO_WS_PIN,    // IIS_LCLK
        .data_out_num = SPK_DATA_PIN, // IIS_DSIN
        .data_in_num = -1         // IIS_DOUT
    };
    i2s_driver_install(AUDIO_I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(AUDIO_I2S_PORT, &pin_config);
    i2s_zero_dma_buffer(AUDIO_I2S_PORT);
}

void audioInit(void)
{
    esp_err_t err;
    //I2S初始化
   i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX, // Receive mode
        .sample_rate = 8000,                         // 8KHz
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, 
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, 
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LOWMED,     // Interrupt level
        .dma_buf_count = 5,                           //5个缓冲区
        .dma_buf_len = 1024                        //单个缓冲区1024Byte（512个16bit数据），8K采样率一共可缓存约0.32S数据
    };
    //引脚设置
    i2s_pin_config_t pin_config = {
        .mck_io_num = -1,
        .bck_io_num = AUDIO_CLK_PIN,   // BCKL
        .ws_io_num = AUDIO_WS_PIN,    // LRCL
        .data_out_num = -1, // not used (only for speakers)
        .data_in_num = AUDIO_DATA_PIN   // DOUT
    };
    err = i2s_driver_install(AUDIO_I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        printf("Failed installing i2s driver: %d\r\n", err);
        return;
    }
    err = i2s_set_pin(AUDIO_I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        printf("Failed setting i2s pin: %d\r\n", err);
        return;
    }
    i2s_zero_dma_buffer(AUDIO_I2S_PORT);
    pBuff = (uint8_t*)audioBuff1; //当前存储指针定位

    //检测
    /*writeAudioDataToBuff();
    vTaskDelay(400);
    writeAudioDataToBuff();
    vTaskDelay(400);
    uint16_t* aBuff = (uint16_t*)&(audioBuff1[50]);

    //printf("audio:%d,%d,%d,%d\n",aBuff[0],aBuff[5],aBuff[10],aBuff[15]);
    if((aBuff[0]!=aBuff[5]) && (aBuff[5]!=aBuff[10]) && (aBuff[10]!=aBuff[15]))
    {
        printf("I2S driver installed.\r\n");
        audio_ready = 1;
    }*/
    audio_ready = 1;
}
//写入音频数据到缓存，返回写入长度
size_t writeAudioDataToBuff(void){
    static uint8_t audioIdleBuff_2 = 0;
    size_t writeSize;
    
    if(audioIdleBuff == 1){ //正在写入BUFF1
        if(audioIdleBuff_2 != audioIdleBuff){
            //刚切换缓存
            audioIdleBuff_2 = audioIdleBuff;
            pBuff = (uint8_t*)audioBuff1;
            audioBuff1Size = 0;
        }
        i2s_read(AUDIO_I2S_PORT, pBuff, 5120, &writeSize, 0);
        if(writeSize > 0){
            //缓存0.32S
            pBuff += writeSize;
            audioBuff1Size = pBuff - (uint8_t*)audioBuff1;
            if(audioBuff1Size >= 5120){
                audioIdleBuff = 2;
            }
            return writeSize;
        }
    }else{ //正在写入BUFF2
        if(audioIdleBuff_2 != audioIdleBuff){
            //刚切换缓存
            audioIdleBuff_2 = audioIdleBuff;
            pBuff = (uint8_t*)audioBuff2;
            audioBuff2Size = 0;
        }
        i2s_read(AUDIO_I2S_PORT, pBuff, 5120, &writeSize, 0);
        if(writeSize > 0){
            //缓存0.32S
            pBuff += writeSize;
            audioBuff2Size = pBuff - (uint8_t*)audioBuff2;
            if(audioBuff2Size >= 5120){
                audioIdleBuff = 1;
            }
            return writeSize;
        }
    }
    return 0;
}
//从缓存中获取音频数据，返回数组指针
uint8_t* getAudioDataFromBuff(void){
    if(audioIdleBuff == 1){ //当前写入BUFF1
        audioDataSize = audioBuff2Size;
        audioBuff2Size = 0;
        return (uint8_t*)audioBuff2;
    }else if(audioIdleBuff == 2){ //当前写入BUFF2
        audioDataSize = audioBuff1Size;
        audioBuff1Size = 0;
        return (uint8_t*)audioBuff1;
    }else{
        audioDataSize = 0;
    }
    return (uint8_t*)audioBuff1;
}

//输出内存状况
void heap_show(void)
{
    printf("-------------------\r\n");
    printf("Remaining memory(SRAM):%dK\r\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL)/1024);
    printf("Maximum continuous memory remaining(SRAM):%dK\r\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)/1024);
    printf("Remaining memory(SPIRAM):%dK\r\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM)/1024);
    printf("Maximum continuous memory remaining(SPIRAM):%dK\r\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)/1024);
    printf("-------------------\r\n");
}

//保存机器人参数
esp_err_t save_robot_info(char *key_)
{

    return ESP_OK;
}
//读取机器人参数
esp_err_t read_robot_info(char *key_)
{
    uint8_t buff[512];
    struct robot_information* esps3_robot_last;
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(my_handle, key_, NULL, &required_size);
    if (err != ESP_OK) return err;
    err = nvs_get_blob(my_handle, key_, (void*)buff, &required_size);
    if (err != ESP_OK) return err;
    esps3_robot_last = (struct robot_information*)buff;

    //Copy
    memcpy(esps3_robot.wifi_ssid,esps3_robot_last->wifi_ssid,strlen(esps3_robot_last->wifi_ssid));
	memcpy(esps3_robot.wifi_pass,esps3_robot_last->wifi_pass,strlen(esps3_robot_last->wifi_pass));
    memcpy(esps3_robot.name,esps3_robot_last->name,strlen(esps3_robot_last->name));
    memcpy(esps3_robot.password,esps3_robot_last->password,strlen(esps3_robot_last->password));
    memcpy(esps3_robot.localIp,esps3_robot_last->localIp,strlen(esps3_robot_last->localIp));
    memcpy(esps3_robot.remoteIp,esps3_robot_last->remoteIp,strlen(esps3_robot_last->remoteIp));
    esps3_robot.localPort = esps3_robot_last->localPort;
	esps3_robot.remotePort = esps3_robot_last->remotePort;
    esps3_robot.ImgSize = esps3_robot_last->ImgSize;
    esps3_robot.ImgQuality = esps3_robot_last->ImgQuality;
    esps3_robot.Vflip = esps3_robot_last->Vflip;
    esps3_robot.Hflip = esps3_robot_last->Hflip;
    esps3_robot.focus = esps3_robot_last->focus;
    esps3_robot.audioEnable = esps3_robot_last->audioEnable;
    esps3_robot.volume = esps3_robot_last->volume;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

static void drive_ledc_init(void)
{

    //设置IO口
    gpio_pad_select_gpio(ledc_pin_motor_l1a);
	gpio_set_direction(ledc_pin_motor_l1b, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(ledc_pin_motor_l1b, GPIO_PULLDOWN_ONLY);

    gpio_pad_select_gpio(ledc_pin_motor_r1a);
	gpio_set_direction(ledc_pin_motor_r1b, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(ledc_pin_motor_r1b, GPIO_PULLDOWN_ONLY);

    gpio_pad_select_gpio(ledc_pin_motor_l1b);
	gpio_set_direction(ledc_pin_motor_l1b, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(ledc_pin_motor_l1b, GPIO_PULLDOWN_ONLY);

    gpio_pad_select_gpio(ledc_pin_motor_r1b);
	gpio_set_direction(ledc_pin_motor_r1b, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(ledc_pin_motor_r1b, GPIO_PULLDOWN_ONLY);

    gpio_set_level(ledc_pin_motor_l1a, 0);
    gpio_set_level(ledc_pin_motor_r1a, 0);
    gpio_set_level(ledc_pin_motor_l1b, 0);
    gpio_set_level(ledc_pin_motor_r1b, 0);

	//配置电机、LED - PWM
    
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = ledc_timer_motor_led,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = ledc_freq_timer1,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = ledc_channel_motor_l1,
        .timer_sel      = ledc_timer_motor_led,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = ledc_pin_motor_l1a,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

	/*ledc_channel.channel = ledc_channel_motor_l2;
	ledc_channel.gpio_num = ledc_pin_motor_l2;
	ledc_channel.duty = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));*/

	ledc_channel.channel = ledc_channel_motor_r1;
	ledc_channel.gpio_num = ledc_pin_motor_r1a;
	ledc_channel.duty = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

	/*ledc_channel.channel = ledc_channel_motor_r2;
	ledc_channel.gpio_num = ledc_pin_motor_r2;
	ledc_channel.duty = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));*/

	ledc_channel.channel = ledc_channel_motor_led;
	ledc_channel.gpio_num = ledc_pin_led;
	ledc_channel.duty = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    //配置舵机
    ledc_timer_config_t ledc_timer2 = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = ledc_timer_servo,
        .duty_resolution  = LEDC_TIMER_14_BIT,
        .freq_hz          = ledc_freq_timer2,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer2));

    ledc_channel_config_t ledc_channel2 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .timer_sel      = ledc_timer_servo,
        .intr_type      = LEDC_INTR_DISABLE,
        .duty           = 1230, 
        .hpoint         = 0
    };
#if defined(ledc_pin_servo1)
    ledc_channel2.channel = ledc_channel_servo1;
	ledc_channel2.gpio_num = ledc_pin_servo1;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel2));
#endif
#if defined(ledc_pin_servo2)
    ledc_channel2.channel = ledc_channel_servo2;
	ledc_channel2.gpio_num = ledc_pin_servo2;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel2));
#endif
#if defined(ledc_pin_servo3)
    ledc_channel2.channel = ledc_channel_servo3;
	ledc_channel2.gpio_num = ledc_pin_servo3;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel2));
#endif
}

// 舵机设置
void servoSet(uint8_t ch,float angle)
{
    float angle2 = angle;
    if((ch == ledc_channel_servo1) && (reverse_s1))
        angle2 = 180 - angle2;
    if((ch == ledc_channel_servo2) && (reverse_s2))
        angle2 = 180 - angle2;
    if((ch == ledc_channel_servo3) && (reverse_s3))
        angle2 = 180 - angle2;
    if((ch == ledc_channel_servo4) && (reverse_s4))
        angle2 = 180 - angle2;

    if((angle2>=0) && (angle2<=180))
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, (int)(410+9.1*angle2));//16384,1.22us
        ledc_update_duty(LEDC_LOW_SPEED_MODE, ch);
    }
}

// 电机设置
void motorSet(uint8_t motor, int spe)
{
    int spe2 = spe / 2;
    if(spe2 > 0)
        spe2 += 127;
    else if(spe2 < 0)
        spe2 -= 127;
    if(motor == MOTOR_L)
    {
        if(reverse_ml1)
            spe2 = 0 - spe2;
        if(spe2>=0)
        {
            ledc_set_pin(ledc_pin_motor_l1b, LEDC_LOW_SPEED_MODE, ledc_channel_motor_l1);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_l1, spe2);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_l1);

            gpio_pad_select_gpio(ledc_pin_motor_l1a);
	        gpio_set_direction(ledc_pin_motor_l1a, GPIO_MODE_OUTPUT);
            gpio_set_pull_mode(ledc_pin_motor_l1a, GPIO_FLOATING);
            gpio_set_level(ledc_pin_motor_l1a, 0);
        }
        else
        {
            ledc_set_pin(ledc_pin_motor_l1a, LEDC_LOW_SPEED_MODE, ledc_channel_motor_l1);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_l1, 0-spe2);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_l1);

            gpio_pad_select_gpio(ledc_pin_motor_l1b);
	        gpio_set_direction(ledc_pin_motor_l1b, GPIO_MODE_OUTPUT);
            gpio_set_pull_mode(ledc_pin_motor_l1b, GPIO_FLOATING);
            gpio_set_level(ledc_pin_motor_l1b, 0);
        }
    }
    else if(motor == MOTOR_R)
    {
        if(reverse_mr1)
            spe2 = 0 - spe2;
        if(spe2>=0)
        {
            ledc_set_pin(ledc_pin_motor_r1b, LEDC_LOW_SPEED_MODE, ledc_channel_motor_r1);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_r1, spe2);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_r1);

            gpio_pad_select_gpio(ledc_pin_motor_r1a);
	        gpio_set_direction(ledc_pin_motor_r1a, GPIO_MODE_OUTPUT);
            gpio_set_pull_mode(ledc_pin_motor_r1a, GPIO_FLOATING);
            gpio_set_level(ledc_pin_motor_r1a, 0);
        }
        else
        {
            ledc_set_pin(ledc_pin_motor_r1a, LEDC_LOW_SPEED_MODE, ledc_channel_motor_r1);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_r1, 0-spe2);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_r1);

            gpio_pad_select_gpio(ledc_pin_motor_r1b);
	        gpio_set_direction(ledc_pin_motor_r1b, GPIO_MODE_OUTPUT);
            gpio_set_pull_mode(ledc_pin_motor_r1b, GPIO_FLOATING);
            gpio_set_level(ledc_pin_motor_r1b, 0);
        }
    }
}

// 照明灯设置
void headlampSet(int bri)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_led, bri);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel_motor_led);
}

void other_init(void)
{
    gpio_pad_select_gpio(out_pin_arms);
	gpio_set_direction(out_pin_arms, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(out_pin_arms, GPIO_FLOATING);
    gpio_set_level(out_pin_arms, 0);

    gpio_pad_select_gpio(out_pin_armsb);
	gpio_set_direction(out_pin_armsb, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(out_pin_armsb, GPIO_FLOATING);
    gpio_set_level(out_pin_armsb, 0);

    //adc_calibration_init();
    //ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    //ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHG_BAT, ADC_EXAMPLE_ATTEN));
}

//电量计算，传入ADC电压值，返回百分比
int electricityCalculation(int vol)
{
  int surplus = 0;
  static uint8_t count = 0;
  static int data[10] = {0};
  int max = 0, min = 10000, average = 0, sum = 0;

  //去除最大值与最小值取平均值
  if(count > 9)
    count = 0;
  data[count] = vol;
  count ++;
  for(uint8_t i = 0; i < 10; i ++)
  {
    if(min > data[i])
        min = data[i];
    if(max < data[i])
        max = data[i];
    sum += data[i];
  }
  average = (sum - (min + max)) / 8;
  vol = average;
  
  if(vol>=2591)//100%,4.2V,2591
    surplus = 100;
  else if(vol>=2505)//90%,4.06,2505
    //surplus = 90 + (int)((vol-4060)/((4200-4060)/10.0));
    surplus = 90 + (int)((vol-2505)/((2591-2505)/10.0));
  else if(vol>=2455)//80%,3.98,2455
    //surplus = 80 + (int)((vol-3980)/((4060-3980)/10.0));
    surplus = 80 + (int)((vol-2455)/((2505-2455)/10.0));
  else if(vol>=2415)//70%,3.92,2415
    //surplus = 70 + (int)((vol-3920)/((3980-3920)/10.0));
    surplus = 70 + (int)((vol-2415)/((2455-2415)/10.0));
  else if(vol>=2383)//60%,3.87,2383
    //surplus = 60 + (int)((vol-3870)/((3920-3870)/10.0));
    surplus = 60 + (int)((vol-2383)/((2415-2383)/10.0));
  else if(vol>=2350)//50%,3.82,2350
    //surplus = 50 + (int)((vol-3820)/((3870-3820)/10.0));
    surplus = 50 + (int)((vol-2350)/((2383-2350)/10.0));
  else if(vol>=2330)//40%,3.79,2330
    //surplus = 40 + (int)((vol-3790)/((3820-3790)/10.0));
    surplus = 40 + (int)((vol-2330)/((2350-2330)/10.0));
  else if(vol>=2320)//30%,3.77,2320
    //surplus = 30 + (int)((vol-3770)/((3790-3770)/10.0));
    surplus = 30 + (int)((vol-2320)/((2330-2320)/10.0));
  else if(vol>=2300)//20%,3.74,2300
    //surplus = 20 + (int)((vol-3740)/((3770-3740)/10.0));
    surplus = 20 + (int)((vol-2300)/((2320-2300)/10.0));
  else if(vol>=2265)//10%,3.68,2265
    //surplus = 10 + (int)((vol-3680)/((3740-3680)/10.0));
    surplus = 10 + (int)((vol-2265)/((2300-2265)/10.0));
  else if(vol>2000)//3.2,2000
    //surplus = (int)((vol-3200)/((3680-3200)/10.0));
    surplus = (int)((vol-2000)/((2265-2000)/10.0));
  else if(vol<=2000)//0%3.2
    surplus = 0;
  return surplus;
}

int get_bat_voltage(void)
{
    int bat = adc1_get_raw(ADC_CHG_BAT);
    //bat = esp_adc_cal_raw_to_voltage(bat, &adc1_chars) * 2;
    return electricityCalculation(bat);
    //return bat;
}

void robotDriveInit(void)
{
    drive_ledc_init();
    other_init();
    motorSet(MOTOR_L, 0);
    motorSet(MOTOR_R, 0);
    servoSet(ledc_channel_servo1, 90);
    servoSet(ledc_channel_servo2, 90);
    servoSet(ledc_channel_servo3, 90);
    headlampSet(0);
}

void robotStop(void)
{
    headlampSet(0);
    motorSet(MOTOR_L, 0);
    motorSet(MOTOR_R, 0);
}






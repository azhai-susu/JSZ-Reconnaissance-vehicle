/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#include "drive.h"


static const char *TAG = "ADC SINGLE";

//static esp_adc_cal_characteristics_t adc1_chars;

int handle_lx = 0;
int handle_ly = 0;
int handle_rx = 0;
int handle_ry = 0;
int charge_voltage = 0;
int bat_voltage = 0;
uint8_t button_brightness = 0;
uint8_t button_focus = 0;
uint8_t button_photograph = 0;
uint8_t button_arm1 = 0;
uint8_t button_arm2 = 0;
uint8_t button_back = 0;
uint8_t button_set = 0;

int SpeedL = 0;
int SpeedR = 0;
int Brightness = 0;

uint8_t audio_volume = 2;

static int adc_raw[2][10];
static int voltage[2][10];
adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc1_cali_handle = NULL;


struct remote_information esps3_remote;
uint8_t audio_isok = 0;

int button_dac_val = 4095;

/*
# 无按键：4095，A、B、C、D、E、F键：3430、2715、2020、1620、492、1260
# 组合键：AB-2370，CD-1130，EF-375
# 组合键：AC-1819，AD-1480，AE-475，AF-1180
*/
uint16_t ADC_BTVAL_A = 3383;
uint16_t ADC_BTVAL_B = 2695;
uint16_t ADC_BTVAL_C = 1999;
uint16_t ADC_BTVAL_D = 1617;
uint16_t ADC_BTVAL_E = 490;
uint16_t ADC_BTVAL_F = 1261;
uint16_t ADC_BTVAL_AB = 2347;
uint16_t ADC_BTVAL_AC = 1801;
uint16_t ADC_BTVAL_AD = 1482;

#define ADC_BTVAL_THRESHOLD 50



/*
    播放I2S初始化
*/
void play_i2s_init(void)
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 8000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LOWMED,
        .dma_buf_count = 5,
        .dma_buf_len = 1024,
        .bits_per_chan = I2S_BITS_PER_SAMPLE_16BIT};
    i2s_pin_config_t pin_config = {
        .mck_io_num = -1,
        .bck_io_num = IIS_SCLK,   // IIS_SCLK
        .ws_io_num = IIS_LCLK,    // IIS_LCLK
        .data_out_num = IIS_DSIN, // IIS_DSIN
        .data_in_num = -1         // IIS_DOUT
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);
    audio_isok = 1;
}


/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static bool adc_init(void)
{
    //-------------ADC1 Init---------------//
    //adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHG_LX, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHG_LY, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHG_BUTTON, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHG_BAT, &config));

    //-------------ADC1 Calibration Init---------------//
    bool do_calibration1 = example_adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_11, &adc1_cali_handle);


    return do_calibration1;
}

uint8_t keyboardRecognition(int key, int adval)
{
    if(adval < (key + ADC_BTVAL_THRESHOLD) && adval > (key - ADC_BTVAL_THRESHOLD))
        return 1;
    return 0;
}

/*
# 无按键：4095，A、B、C、D、E、F键：3430、2715、2020、1620、492、1260
# 组合键：AB-2370，CD-1130，EF-375
# 组合键：AC-1819，AD-1480，AE-475，AF-1180
*/
void get_keyboard(void)
{
    static uint8_t button_focus_last = 0;
    static uint8_t button_photograph_last = 0;

    int handle_lx_1,handle_ly_1,handle_button_1,bat_1,bat_2;
    
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHG_LX, &handle_lx_1));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHG_LY, &handle_ly_1));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHG_BUTTON, &handle_button_1));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHG_BAT, &bat_1));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, bat_1, &bat_2));
    bat_2 *= 2;
    handle_lx_1 /= 32;
    handle_ly_1 /= 32;

    button_dac_val = handle_button_1;

    //printf("LX: %d, LY: %d, BUTTON: %d, BAT: %d\r\n", handle_lx_1,handle_ly_1,handle_button_1,bat_2);
    bat_voltage = bat_2;

    if(handle_lx_1 > 77)
        handle_lx = handle_lx_1 - 77;
    else if(handle_lx_1 < 50)
        handle_lx = handle_lx_1 - 50;
    else
        handle_lx = 0;
    
    if(handle_ly_1 > 77)
        handle_ly = handle_ly_1 - 77;
    else if(handle_ly_1 < 50)
        handle_ly = handle_ly_1 - 50;
    else
        handle_ly = 0;

    //获取键值
    if(keyboardRecognition(ADC_BTVAL_A, handle_button_1))
    {
        if(button_focus_last == 0)
            button_focus = 1;
        button_focus_last = 1;
    }
    else
    {
        button_focus = 0;
        button_focus_last = 0;
    }

    if(keyboardRecognition(ADC_BTVAL_B, handle_button_1))
    {
        if(button_photograph_last == 0)
            button_photograph = 1;
        button_photograph_last = 1;
    }
    else
    {
        button_photograph = 0;
        button_photograph_last = 0;
    }

    if(keyboardRecognition(ADC_BTVAL_C, handle_button_1))
    {
        button_set = 1;
        button_brightness = 1;
    }
    else if(keyboardRecognition(ADC_BTVAL_E, handle_button_1))
        button_brightness = 2;
    else
        button_brightness = 0;

    if(keyboardRecognition(ADC_BTVAL_D, handle_button_1))
        handle_ry = 50;
    else if(keyboardRecognition(ADC_BTVAL_F, handle_button_1))
        handle_ry = -50;
    else
        handle_ry = 0;

    if(keyboardRecognition(ADC_BTVAL_AB, handle_button_1))
        button_back = 1;
    else
        button_back = 0;

    if(keyboardRecognition(ADC_BTVAL_AC, handle_button_1))
        button_arm1 = 1;
    else
        button_arm1 = 0;

    if(keyboardRecognition(ADC_BTVAL_AD, handle_button_1))
        button_arm2 = 1;
    else
        button_arm2 = 0;

    //灯光控制
    if(button_brightness == 1)
    {
        Brightness += 64;
        if(Brightness >= 255)
            Brightness = 255;
        button_brightness = 0;
    }else if(button_brightness == 2){
        Brightness -= 64;
        if(Brightness <= 0)
            Brightness = 0;
        button_brightness = 0;
    }

    int bcL = 0;
    int bcR = 0;
    int rockerMin = 20;//摇杆盲区

    //判断8个方向
    if((handle_ly>rockerMin)&&(handle_lx>(0-rockerMin))&&(handle_lx<rockerMin)){//正前方
        SpeedL = handle_ly * 5 + 6 - bcL;
        SpeedR = handle_ly * 5 + 6 - bcR;

    }else if((handle_ly<(0-rockerMin))&&(handle_lx>(0-rockerMin))&&(handle_lx<rockerMin)) {//正后方
        SpeedL = handle_ly * 5 - 6 + bcL;
        SpeedR = handle_ly * 5 - 6 + bcR;
    }else if((handle_lx<(0-rockerMin))&&(handle_ly>(0-rockerMin))&&(handle_ly<rockerMin)) {//正左方
        SpeedL = handle_lx * 5 - 6 + bcL;
        SpeedR = 0 - (handle_lx * 5 - 6) - bcR;
    }else if((handle_lx>rockerMin)&&(handle_ly>(0-rockerMin))&&(handle_ly<rockerMin)) {//正右方
        SpeedL = handle_lx * 5 + 6 - bcL;
        SpeedR = 0 - (handle_lx * 5 + 6) + bcR;
    }else if((handle_ly>rockerMin)&&(handle_lx<(0-rockerMin))) {//前左
        SpeedL = 0;
        SpeedR = handle_ly * 5 + 6;
    }else if((handle_ly>rockerMin)&&(handle_lx>rockerMin)) {//前右
        SpeedL = handle_ly * 5 + 6;
        SpeedR = 0;
    }else if((handle_ly<(0-rockerMin))&&(handle_lx<(0-rockerMin))) {//后左
        SpeedL = 0;
        SpeedR = handle_ly * 5 - 6;
    }else if((handle_ly<(0-rockerMin))&&(handle_lx>rockerMin)) {//后右
        SpeedL = handle_ly * 5 - 6;
        SpeedR = 0;
    }else{
        SpeedL = 0;
        SpeedR = 0;
    }
}

void audio_play(uint8_t *adata, uint32_t datasize)
{
    uint32_t cnt;
    if(audio_isok)
        i2s_write(I2S_NUM, adata, datasize, &cnt, 1000 / portTICK_PERIOD_MS);//输出数据到I2S  就实现了播放
}
void drive_init(void)
{
    //ADC1 config
    bool do_calibration1 = adc_init();
    play_i2s_init();
}


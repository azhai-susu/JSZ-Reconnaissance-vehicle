/**
 * @file lv_port_disp_templ.c
 *
 */

 /*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"

/*********************
 *      DEFINES
 *********************/
static const char *TAG = "lv_port_disp";

esp_lcd_panel_handle_t panel_handle_2 = NULL;

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
// we use two semaphores to sync the VSYNC event and the LVGL task, to avoid potential tearing effect
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
SemaphoreHandle_t sem_vsync_end;
SemaphoreHandle_t sem_gui_ready;
#endif

/**********************
 *      MACROS
 **********************/
static bool rgb_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data);
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_disp_init(void)
{
    static lv_disp_drv_t disp_drv;      // contains callback functions

#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    /* Create semaphores */
    sem_vsync_end = xSemaphoreCreateBinary();
    assert(sem_vsync_end);
    sem_gui_ready = xSemaphoreCreateBinary();
    assert(sem_gui_ready);
#endif

    if(GPIO_LCD_BL != GPIO_NUM_NC){
        gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << GPIO_LCD_BL
        };
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    }

    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * LCD_WIDTH,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = GPIO_NUM_NC,
        .pclk_gpio_num = GPIO_LCD_PCLK,
        .vsync_gpio_num = GPIO_LCD_VSYNC,
        .hsync_gpio_num = GPIO_LCD_HSYNC,
        .de_gpio_num = GPIO_LCD_DE,
        .data_gpio_nums = {
            GPIO_LCD_B0, GPIO_LCD_B1, GPIO_LCD_B2, GPIO_LCD_B3, GPIO_LCD_B4,         
            GPIO_LCD_G0, GPIO_LCD_G1, GPIO_LCD_G2, GPIO_LCD_G3, GPIO_LCD_G4, GPIO_LCD_G5,
            GPIO_LCD_R0, GPIO_LCD_R1, GPIO_LCD_R2, GPIO_LCD_R3, GPIO_LCD_R4,
        },
        .timings = {
            .h_res = LCD_WIDTH,
            .v_res = LCD_HEIGHT,
            // The following parameters should refer to LCD spec
        #if LCD_4r3_480x272
            .pclk_hz = 13 * 1000 * 1000,
            .hsync_back_porch = 42,
            .hsync_front_porch = 1,
            .hsync_pulse_width = 1,
            .vsync_back_porch = 12,
            .vsync_front_porch = 3,
            .vsync_pulse_width = 1,
        #endif
            .flags.pclk_active_neg = true, // RGB data is clocked out on falling edge
        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM
#if CONFIG_EXAMPLE_DOUBLE_FB
        .flags.double_fb = true,   // allocate double frame buffer
#endif // CONFIG_EXAMPLE_DOUBLE_FB
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    /* Register event callbacks */
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = rgb_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &disp_drv));

    /* Initialize RGB LCD panel */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(GPIO_LCD_BL, 1);
    ESP_LOGI(TAG, "LCD resolution: %dx%d", LCD_WIDTH, LCD_HEIGHT);

    /* initialize LVGL draw buffers */
    static lv_disp_draw_buf_t disp_buf;
    void *buf1 = NULL;
    void *buf2 = NULL;
#if CONFIG_EXAMPLE_DOUBLE_FB
    ESP_LOGI(TAG, "Use frame buffers as LVGL draw buffers");
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_WIDTH * LCD_HEIGHT);
#else
    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers from PSRAM");
    uint16_t fact = 120;
    buf1 = heap_caps_malloc(LCD_WIDTH * fact * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);  // MALLOC_CAP_SPIRAM
    assert(buf1);
#if 1
    buf2 = heap_caps_malloc(LCD_WIDTH * fact * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_WIDTH * fact);
#else 
   lv_disp_draw_buf_init(&disp_buf, buf1, NULL, LCD_WIDTH * fact);
#endif
#endif // CONFIG_EXAMPLE_DOUBLE_FB

    /* Register display driver to LVGL */
    lv_disp_drv_init(&disp_drv);   

    /*Set the resolution of the display*/
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = lvgl_flush_cb;
    /*Set a display buffer*/
    disp_drv.draw_buf = &disp_buf;

    disp_drv.user_data = panel_handle;

    ////////////////////
    panel_handle_2 = panel_handle;

#if CONFIG_EXAMPLE_DOUBLE_FB
    disp_drv.full_refresh = true; // the full_refresh mode can maintain the synchronization between the two frame buffers
#endif

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static bool rgb_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    if (xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE) {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken);
    }
#endif
    return high_task_awoken == pdTRUE;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    xSemaphoreGive(sem_gui_ready);
    xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
#endif
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}

/**
 * @brief Task to generate ticks for LVGL.
 * 
 * @param pvParam Not used. 
 */
static void lv_tick_inc_cb(void *data)
{
    uint32_t tick_inc_period_ms = *((uint32_t *) data);

    lv_tick_inc(tick_inc_period_ms);
}

/**
 * @brief Create tick task for LVGL.
 * 
 * @return esp_err_t 
 */
esp_err_t lv_port_tick_init(void)
{
    static const uint32_t tick_inc_period_ms = 2;
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = lv_tick_inc_cb,
            .arg = &tick_inc_period_ms,
            // .dispatch_method = ESP_TIMER_TASK,
            .name = "lvgl_tick",     /* name is optional, but may help identify the timer when debugging */
            // .skip_unhandled_events = true,
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* The timer has been created but is not running yet. Start the timer now */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, tick_inc_period_ms * 1000));

    return ESP_OK;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif

/**
 * @file disp_driver.c
 */

#include "disp_driver.h"

void disp_driver_init(void)
{
    lv_port_disp_init();
}

void disp_driver_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle_2, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

void disp_driver_rounder(lv_disp_drv_t * disp_drv, lv_area_t * area)
{

}

void disp_driver_set_px(lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
    lv_color_t color, lv_opa_t opa)
{

}

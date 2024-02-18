if(ESP_PLATFORM)

file(GLOB SOURCES *.c)
set(LVGL_INCLUDE_DIRS . lvgl_tft)
list(APPEND SOURCES "lvgl_tft/disp_driver.c")

#@todo add SimleInclude macro here

# Include only the source file of the selected
# display controller.

list(APPEND SOURCES "lvgl_tft/bsp_i2c.c")
list(APPEND SOURCES "lvgl_tft/lv_port_disp.c")



# Add touch driver to compilation only if it is selected in menuconfig

list(APPEND SOURCES "lvgl_touch/touch_driver.c")
list(APPEND LVGL_INCLUDE_DIRS lvgl_touch)

# Include only the source file of the selected
# touch controller.
list(APPEND SOURCES "lvgl_touch/gt911.c")
list(APPEND SOURCES "lvgl_touch/lv_port_indev.c")



idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${LVGL_INCLUDE_DIRS}
                       REQUIRES lvgl)

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_LVGL_H_INCLUDE_SIMPLE")

else()
    message(FATAL_ERROR "LVGL ESP32 drivers: ESP_PLATFORM is not defined. Try reinstalling ESP-IDF.")
endif()

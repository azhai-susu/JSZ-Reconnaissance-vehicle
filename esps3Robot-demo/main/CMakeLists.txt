set(COMPONENT_SRCS 
    "app_main.c"
    "app_camera.c"
    "page_cam.c"
    "app_network.c"
    "mUdpTransmission.c"
    "app_drive.c"
    "page_imu.c"
    "app_led.c"
    "wav_player.c"
    "file_manager.c"
    )
set(COMPONENT_ADD_INCLUDEDIRS )

register_component()
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)


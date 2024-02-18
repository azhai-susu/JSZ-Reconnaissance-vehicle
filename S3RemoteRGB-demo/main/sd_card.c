/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/

// This example uses SPI peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "drive.h"

static const char *TAG = "SD Card";


extern uint32_t get_photoCount(void);

uint8_t sd_isok = 0;
sdmmc_card_t *card;
sdmmc_host_t host = SDSPI_HOST_DEFAULT();
const char mount_point[] = MOUNT_POINT;


void sd_unmount(void)
{
    if(sd_isok)
    {
        esp_vfs_fat_sdcard_unmount(mount_point, card);
        ESP_LOGI(TAG, "sdCard unmounted");
        spi_bus_free(host.slot);
    }
}
uint8_t file_save(uint8_t *buf, size_t len)
{
    if(sd_isok)
    {
        char path_photo[50];
        uint32_t count = get_photoCount();
        sprintf(path_photo,"%s/photo-%ld.jpg",MOUNT_POINT,count);
        ESP_LOGI(TAG, "Opening file %s", path_photo);
        FILE *f = fopen(path_photo, "w");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            return 0;
        }
        fwrite(buf, 1, len, f);
        fclose(f);
        return 1;
    }
    return 0;
}

uint8_t sdcard_init(void)
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    //sdmmc_card_t *card;
    //const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");
    ESP_LOGI(TAG, "Using SPI peripheral");
    host.slot = 2;//VSPI

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return 0;
    }
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return 0;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    sd_isok = 1;
    ESP_LOGI(TAG, "SD card initialization completed");
    return 1;
}


#ifndef _wav_player_h_
#define _wav_player_h_
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

extern bool playing;
void play_spiffs_name(char *file_name);
#ifdef __cplusplus
}
#endif
#endif
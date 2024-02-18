
#include "wav_player.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "app_drive.h"
#include <sys/stat.h>

#define TAG "WAV_PLAYER"

char path_buf[256] = {0};
char **g_file_list = NULL;
uint16_t g_file_num = 0;
bool playing = false;
typedef struct
{
	// The "RIFF" chunk descriptor
	uint8_t ChunkID[4];
	int32_t ChunkSize;
	uint8_t Format[4];
	// The "fmt" sub-chunk
	uint8_t Subchunk1ID[4];
	int32_t Subchunk1Size;
	int16_t AudioFormat;
	int16_t NumChannels;
	int32_t SampleRate;
	int32_t ByteRate;
	int16_t BlockAlign;
	int16_t BitsPerSample;
	// The "data" sub-chunk
	uint8_t Subchunk2ID[4];
	int32_t Subchunk2Size;
} wav_header_t;

esp_err_t play_wav(const char *filepath)
{

	all_i2s_deinit();//先把i2s的驱动卸载掉 因为在这之前是把i2s初始化为录音了 现在要播放 公用的引脚就不能播放  需要卸载掉重新初始化为播放模式
	play_i2s_init();//初始化播放模式的i2s
	FILE *fd = NULL;
	struct stat file_stat;

	if (stat(filepath, &file_stat) == -1)//先找找这个文件是否存在
	{
		ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
		all_i2s_deinit();
		audioInit();
		return ESP_FAIL;//如果不存在就继续录音
	}

	ESP_LOGI(TAG, "file stat info: %s (%ld bytes)...", filepath, file_stat.st_size);
	fd = fopen(filepath, "r");

	if (NULL == fd)
	{
		ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
		all_i2s_deinit();
		audioInit();
		return ESP_FAIL;
	}
	const size_t chunk_size = 4096;
	uint8_t *buffer = malloc(chunk_size);

	if (NULL == buffer)
	{
		ESP_LOGE(TAG, "audio data buffer malloc failed");
		all_i2s_deinit();
		audioInit();
		fclose(fd);
		return ESP_FAIL;
	}

	/**
	 * read head of WAV file
	 */
	wav_header_t wav_head;
	int len = fread(&wav_head, 1, sizeof(wav_header_t), fd);//读取wav文件的文件头
	if (len <= 0)
	{
		ESP_LOGE(TAG, "Read wav header failed");
		all_i2s_deinit();
		audioInit();
		fclose(fd);
		return ESP_FAIL;
	}
	if (NULL == strstr((char *)wav_head.Subchunk1ID, "fmt") &&
		NULL == strstr((char *)wav_head.Subchunk2ID, "data"))
	{
		ESP_LOGE(TAG, "Header of wav format error");
		all_i2s_deinit();
		audioInit();
		fclose(fd);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "frame_rate=%d, ch=%d, width=%d", wav_head.SampleRate, wav_head.NumChannels, wav_head.BitsPerSample);
	/**
	 * read wave data of WAV file
	 */
	size_t write_num = 0;
	size_t cnt;
	ESP_LOGI(TAG, "set clock");
	i2s_set_clk(1, wav_head.SampleRate, wav_head.BitsPerSample, 1);//根据该wav文件的各种参数来配置一下i2S的clk 采样率等等
	ESP_LOGI(TAG, "write data");
	do
	{
		/* Read file in chunks into the scratch buffer */
		len = fread(buffer, 1, chunk_size, fd);
		if (len <= 0)
		{
			break;
		}
		i2s_write(1, buffer, len, &cnt, 1000 / portTICK_PERIOD_MS);//输出数据到I2S  就实现了播放
		write_num += len;
	} while (1);
	fclose(fd);
	ESP_LOGI(TAG, "File reading complete, total: %d bytes", write_num);

	all_i2s_deinit();
	// audioInit();
	return ESP_OK;
}

void play_spiffs_name(char *file_name)
{
	playing = true;
	sprintf(path_buf, "%s/%s", "/spiffs/wav", file_name);
	play_wav(path_buf);
	audioInit();
	playing = false;
}


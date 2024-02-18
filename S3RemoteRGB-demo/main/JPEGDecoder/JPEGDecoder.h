#ifndef JPEGDECODER_H
#define JPEGDECODER_H

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "picojpeg.h"

enum {
	JPEG_ARRAY = 0,
	JPEG_FS_FILE,
	JPEG_SD_FILE
};


extern uint16_t *JPEGDecoder_pImage;
//JPEGDecoder *thisPtr;

extern int JPEGDecoder_width;
extern int JPEGDecoder_height;
extern int JPEGDecoder_comps;
extern int JPEGDecoder_MCUSPerRow;
extern int JPEGDecoder_MCUSPerCol;
extern pjpeg_scan_type_t JPEGDecoder_scanType;
extern int JPEGDecoder_MCUWidth;
extern int JPEGDecoder_MCUHeight;
extern int JPEGDecoder_MCUx;
extern int JPEGDecoder_MCUy;
	

int JPEGDecoder_available(void);
int JPEGDecoder_read(void);
int JPEGDecoder_readSwappedBytes(void);
	
int JPEGDecoder_decodeFile (const char *pFilename);
//int JPEGDecoder_decodeFile (const String& pFilename);

int JPEGDecoder_decodeArray(const uint8_t array[], uint32_t  array_size);
void JPEGDecoder_abort(void);


#endif // JPEGDECODER_H

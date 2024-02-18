
#include "esp_log.h"
#include "JPEGDecoder.h"
#include "picojpeg.h"
#include "string.h"
#include <stdio.h>


//------------------------------------------------------------------------------
typedef unsigned char uint8;
typedef unsigned int uint;
//------------------------------------------------------------------------------
//#define DEBUG
//------------------------------------------------------------------------------
#ifndef jpg_min
	#define jpg_min(a,b)     (((a) < (b)) ? (a) : (b))
#endif
#ifndef min
	#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#define ESP32
#define DEBUG

static const char* TAG = "JPEGDecoder";

//private:
pjpeg_scan_type_t scan_type;
pjpeg_image_info_t image_info;

int is_available = 0;
int mcu_x = 0;
int mcu_y = 0;
uint g_nInFileSize;
uint g_nInFileOfs;
uint row_pitch;
uint decoded_width, decoded_height;
uint row_blocks_per_mcu, col_blocks_per_mcu;
uint8 status;
uint8 jpg_source = 0;
uint8_t* jpg_data;

uint8_t JPEGDecoder_pjpeg_callback(uint8_t* pBuf, uint8_t buf_size, uint8_t *pBytes_actually_read, void *pCallback_data);
uint8_t JPEGDecoder_pjpeg_need_bytes_callback(uint8_t* pBuf, uint8_t buf_size, uint8_t *pBytes_actually_read, void *pCallback_data);
int JPEGDecoder_decode_mcu(void);
int JPEGDecoder_decodeCommon(void);

//public:
uint16_t image_buff[16*16];
uint16_t *JPEGDecoder_pImage = image_buff;
//JPEGDecoder *thisPtr;

int JPEGDecoder_width;
int JPEGDecoder_height;
int JPEGDecoder_comps;
int JPEGDecoder_MCUSPerRow;
int JPEGDecoder_MCUSPerCol;
pjpeg_scan_type_t JPEGDecoder_scanType;
int JPEGDecoder_MCUWidth;
int JPEGDecoder_MCUHeight;
int JPEGDecoder_MCUx;
int JPEGDecoder_MCUy;


uint8_t JPEGDecoder_pjpeg_callback(uint8_t* pBuf, uint8_t buf_size, uint8_t *pBytes_actually_read, void *pCallback_data) {
	JPEGDecoder_pjpeg_need_bytes_callback(pBuf, buf_size, pBytes_actually_read, pCallback_data);
	return 0;
}


uint8_t JPEGDecoder_pjpeg_need_bytes_callback(uint8_t* pBuf, uint8_t buf_size, uint8_t *pBytes_actually_read, void *pCallback_data) {
	uint n;
	n = jpg_min(g_nInFileSize - g_nInFileOfs, buf_size);

	if (jpg_source == JPEG_ARRAY) { // We are handling an array
		uint8_t *p1 = pBuf, *p2;
		for (p2 = p1 + n; p1 < p2; p1 ++) {
			*p1 = *jpg_data;
			jpg_data ++;
		}
	}

	*pBytes_actually_read = (uint8_t)(n);
	g_nInFileOfs += n;
	return 0;
}

int JPEGDecoder_decode_mcu(void) {

	status = pjpeg_decode_mcu();

	if (status) {
		is_available = 0 ;

		if (status != PJPG_NO_MORE_BLOCKS) {
			#ifdef DEBUG
			ESP_LOGE(TAG,"pjpeg_decode_mcu() failed with status: %d\n",status);
			#endif

			return -1;
		}
	}
	return 1;
}


int JPEGDecoder_read(void) {
	int y, x;
	uint16_t *pDst_row;

	if(is_available == 0 || mcu_y >= image_info.m_MCUSPerCol) {
		//abort();
		return 0;
	}
	
	// Copy MCU's pixel blocks into the destination bitmap.
	pDst_row = JPEGDecoder_pImage;
	for (y = 0; y < image_info.m_MCUHeight; y += 8) {

		const int by_limit = jpg_min(8, image_info.m_height - (mcu_y * image_info.m_MCUHeight + y));

		for (x = 0; x < image_info.m_MCUWidth; x += 8) {
			uint16_t *pDst_block = pDst_row + x;

			// Compute source byte offset of the block in the decoder's MCU buffer.
			uint src_ofs = (x * 8U) + (y * 16U);
			const uint8_t *pSrcR = image_info.m_pMCUBufR + src_ofs;
			const uint8_t *pSrcG = image_info.m_pMCUBufG + src_ofs;
			const uint8_t *pSrcB = image_info.m_pMCUBufB + src_ofs;

			const int bx_limit = jpg_min(8, image_info.m_width - (mcu_x * image_info.m_MCUWidth + x));

			if (image_info.m_scanType == PJPG_GRAYSCALE) {
				int bx, by;
				for (by = 0; by < by_limit; by++) {
					uint16_t *pDst = pDst_block;

					for (bx = 0; bx < bx_limit; bx++) {
						*pDst++ = (*pSrcR & 0xF8) << 8 | (*pSrcR & 0xFC) <<3 | *pSrcR >> 3;
						pSrcR++;
					}

					pSrcR += (8 - bx_limit);

					pDst_block += row_pitch;
				}
			}
			else {
				int bx, by;
				for (by = 0; by < by_limit; by++) {
					uint16_t *pDst = pDst_block;

					for (bx = 0; bx < bx_limit; bx++) {
						*pDst++ = (*pSrcR & 0xF8) << 8 | (*pSrcG & 0xFC) <<3 | *pSrcB >> 3;
						pSrcR++; pSrcG++; pSrcB++;
					}

					pSrcR += (8 - bx_limit);
					pSrcG += (8 - bx_limit);
					pSrcB += (8 - bx_limit);

					pDst_block += row_pitch;
				}
			}
		}
		pDst_row += (row_pitch * 8);
	}

	JPEGDecoder_MCUx = mcu_x;
	JPEGDecoder_MCUy = mcu_y;

	mcu_x++;
	if (mcu_x == image_info.m_MCUSPerRow) {
		mcu_x = 0;
		mcu_y++;
	}

	if(JPEGDecoder_decode_mcu()==-1) is_available = 0 ;

	return 1;
}

int JPEGDecoder_readSwappedBytes(void) {
	int y, x;
	uint16_t *pDst_row;

	if(is_available == 0 || mcu_y >= image_info.m_MCUSPerCol) {
		//abort();
		return 0;
	}
	
	// Copy MCU's pixel blocks into the destination bitmap.
	pDst_row = JPEGDecoder_pImage;
	for (y = 0; y < image_info.m_MCUHeight; y += 8) {

		const int by_limit = jpg_min(8, image_info.m_height - (mcu_y * image_info.m_MCUHeight + y));

		for (x = 0; x < image_info.m_MCUWidth; x += 8) {
			uint16_t *pDst_block = pDst_row + x;

			// Compute source byte offset of the block in the decoder's MCU buffer.
			uint src_ofs = (x * 8U) + (y * 16U);
			const uint8_t *pSrcR = image_info.m_pMCUBufR + src_ofs;
			const uint8_t *pSrcG = image_info.m_pMCUBufG + src_ofs;
			const uint8_t *pSrcB = image_info.m_pMCUBufB + src_ofs;

			const int bx_limit = jpg_min(8, image_info.m_width - (mcu_x * image_info.m_MCUWidth + x));

			if (image_info.m_scanType == PJPG_GRAYSCALE) {
				int bx, by;
				for (by = 0; by < by_limit; by++) {
					uint16_t *pDst = pDst_block;

					for (bx = 0; bx < bx_limit; bx++) {

						*pDst++ = (*pSrcR & 0xF8) | (*pSrcR & 0xE0) >> 5 | (*pSrcR & 0xF8) << 5 | (*pSrcR & 0x1C) << 11;

						pSrcR++;
					}
				}
			}
			else {
				int bx, by;
				for (by = 0; by < by_limit; by++) {
					uint16_t *pDst = pDst_block;

					for (bx = 0; bx < bx_limit; bx++) {

						*pDst++ = (*pSrcR & 0xF8) | (*pSrcG & 0xE0) >> 5 | (*pSrcB & 0xF8) << 5 | (*pSrcG & 0x1C) << 11;

						pSrcR++; pSrcG++; pSrcB++;
					}

					pSrcR += (8 - bx_limit);
					pSrcG += (8 - bx_limit);
					pSrcB += (8 - bx_limit);

					pDst_block += row_pitch;
				}
			}
		}
		pDst_row += (row_pitch * 8);
	}

	JPEGDecoder_MCUx = mcu_x;
	JPEGDecoder_MCUy = mcu_y;

	mcu_x++;
	if (mcu_x == image_info.m_MCUSPerRow) {
		mcu_x = 0;
		mcu_y++;
	}

	if(JPEGDecoder_decode_mcu()==-1) is_available = 0 ;

	return 1;
}


// Generic file call for SD or SPIFFS, uses leading / to distinguish SPIFFS files
int JPEGDecoder_decodeFile(const char *pFilename){
	return -1;
}


int JPEGDecoder_decodeArray(const uint8_t array[], uint32_t  array_size) {

	jpg_source = JPEG_ARRAY; // We are not processing a file, use arrays

	g_nInFileOfs = 0;

	jpg_data = (uint8_t *)array;

	g_nInFileSize = array_size;

	return JPEGDecoder_decodeCommon();
}


int JPEGDecoder_decodeCommon(void) {

	JPEGDecoder_width = 0;
	JPEGDecoder_height = 0;
	JPEGDecoder_comps = 0;
	JPEGDecoder_MCUSPerRow = 0;
	JPEGDecoder_MCUSPerCol = 0;
	JPEGDecoder_scanType = (pjpeg_scan_type_t)0;
	JPEGDecoder_MCUWidth = 0;
	JPEGDecoder_MCUHeight = 0;

	status = pjpeg_decode_init(&image_info, JPEGDecoder_pjpeg_callback, NULL, 0);

	if (status) {
		#ifdef DEBUG
			ESP_LOGE(TAG,"pjpeg_decode_init() failed with status %d\n",status);

			if (status == PJPG_UNSUPPORTED_MODE) {
				ESP_LOGE(TAG,"Progressive JPEG files are not supported.\n");
			}
		#endif

		return 0;
	}

	decoded_width =  image_info.m_width;
	decoded_height =  image_info.m_height;
	
	row_pitch = image_info.m_MCUWidth;
	JPEGDecoder_pImage = image_buff;
	memset(JPEGDecoder_pImage , 0 , image_info.m_MCUWidth * image_info.m_MCUHeight * 2);

	row_blocks_per_mcu = image_info.m_MCUWidth >> 3;
	col_blocks_per_mcu = image_info.m_MCUHeight >> 3;

	is_available = 1 ;

	JPEGDecoder_width = decoded_width;
	JPEGDecoder_height = decoded_height;
	JPEGDecoder_comps = 1;
	JPEGDecoder_MCUSPerRow = image_info.m_MCUSPerRow;
	JPEGDecoder_MCUSPerCol = image_info.m_MCUSPerCol;
	JPEGDecoder_scanType = image_info.m_scanType;
	JPEGDecoder_MCUWidth = image_info.m_MCUWidth;
	JPEGDecoder_MCUHeight = image_info.m_MCUHeight;

	return JPEGDecoder_decode_mcu();
}

void JPEGDecoder_abort(void) {

	mcu_x = 0 ;
	mcu_y = 0 ;
	is_available = 0;
}

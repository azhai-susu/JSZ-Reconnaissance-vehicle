/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#ifndef MUDPTRANSMISSION_H
#define MUDPTRANSMISSION_H

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include <string.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


#define PACKET_STANDARD_LEN 1200 //UDP标准数据包大小，含数据头28Byte
#define PACKET_HEADER_LEN 28 //数据包头长度


extern char udpAddress[20];
extern const uint16_t udpPort;


uint32_t getFrameId(void);
uint16_t getPacketCount(void);
uint32_t getPictureData(uint8_t * buff);
uint32_t getAudioData(uint8_t * buff);
uint32_t  getMsgData(uint8_t * buff);
uint8_t importData(uint8_t *udpbuff, uint16_t len);
uint16_t generatingPackets(uint32_t msgId, uint8_t msgType, uint8_t *sdata, uint16_t slen, uint8_t **packet);
uint8_t* parsingPackets(uint8_t *buf, uint16_t len, uint32_t *msg_id, uint8_t *msg_type);
uint32_t dataJoin(uint8_t* udpSf, uint32_t f_id, uint8_t* buff1, uint32_t len1, uint8_t* buff2, uint32_t len2, uint8_t* buff3, uint32_t len3);
uint8_t udpSendBuffData(uint8_t* udpSf, int sock, struct sockaddr_storage source_addr);
uint8_t udpSendBuffDataPacket(uint8_t* udpSf, int sock, struct sockaddr_storage source_addr,uint16_t packet);

#endif
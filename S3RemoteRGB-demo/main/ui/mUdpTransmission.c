/*
    项目名称：ESP32S3-4R7图传遥控器
    作者：阿宅酥酥/技术宅物语
    日期：2024年2月
    公司：河源创易电子有限公司
    淘宝：技术宅物语
*/
#include "mUdpTransmission.h"


#define TAG "Transmission"

//////////////////////////

uint16_t packetHeaderLen = 28; //数据头长度
uint32_t frameId = 0xffffffff; //帧号
uint32_t frameIdOld = 0xffffffff;
uint32_t frameSize = 0; //不含数据头的帧尺寸
uint32_t contentLenA = 0; //第一部分数据尺寸
uint32_t contentLenB = 0; //第二部分数据尺寸
uint32_t contentLenC = 0; //第三部分数据尺寸
uint16_t packetCount = 0; //总包数
uint16_t packetStandardLen = 0; //标准包长
uint16_t checkSum = 0; //数据包校验值
uint16_t packetId = 0; //当前数据包号
uint16_t packetLen = 0; //当前数据包长度
uint16_t resendQuantity = 0; //需要重发的数据包数量

uint8_t rFinish = 0;

uint8_t dataBuff[20480];

uint8_t packetState[60] = {0}; //记录当前帧数据包接收状态

uint8_t tx_Buff[256] = {0}; //开辟256字节发送缓存

uint32_t frame_id = 0;
uint16_t packet_count = 0;

//////////////////////////
//WiFiUDP udp;
char udpAddress[20]={0};
const uint16_t udpPort = 10000;

//uint8_t rx_buffer[PACKET_STANDARD_LEN];
uint8_t tx_buffer[PACKET_STANDARD_LEN] = {0};

uint32_t getFrameId(void){
    return frame_id;
}
uint16_t getPacketCount(void){
    return packet_count;
}

uint32_t getPictureData(uint8_t * buff){
    if(rFinish){
        memcpy(buff, dataBuff, contentLenA);
    }else{
        return 0;
    }
    return contentLenA;
}

uint32_t getAudioData(uint8_t * buff){
    if(rFinish){
        memcpy(buff, dataBuff + contentLenA, contentLenB);
    }else{
        return 0;
    }
    return contentLenB;
}

uint32_t  getMsgData(uint8_t * buff){
    if(rFinish){
        memcpy(buff, dataBuff + contentLenA + contentLenB, contentLenC);
    }else{
        return 0;
    }
    return contentLenC;
}

//导入接收到的新数据包，失败返回0，接收返回1，接收到最后一包但不完整返回2，接收完整返回3
uint8_t importData(uint8_t *udpbuff, uint16_t len){
    uint16_t rxLen = len;
    //判断数组长度是否符合规范
    if(rxLen >= packetHeaderLen){
        //CRC校验
        uint16_t crc = 0;
        for(uint16_t ln = 0; ln < packetHeaderLen - 2; ln++){
            crc += udpbuff[ln];
        }
        for(uint16_t ln = packetHeaderLen; ln < rxLen; ln++){
            crc += udpbuff[ln];
        }
        packetLen = udpbuff[22] * 256 + udpbuff[23];
        uint16_t crc1 = udpbuff[26] * 256 + udpbuff[27];
        //Serial.printf("crc:%d, udpbuffCrc:%d, packetLen:%d, rxLen:%d.\n", crc, crc1, packetLen, rxLen);

        if((crc == crc1) && (packetLen == rxLen)) {
            //数据包参数解析
            uint32_t fId = ((uint32_t)udpbuff[0] << 24) | ((uint32_t)udpbuff[1] << 16) | ((uint32_t)udpbuff[2] << 8) | (uint32_t)udpbuff[3];
            //当前帧号与上一次不同，表示新的一帧开始，重新记录参数
            if (fId != frameId){//换帧
                frameIdOld = frameId;
                frameId = fId;
                frameSize = ((uint32_t)udpbuff[4] << 24) | ((uint32_t)udpbuff[5] << 16) | ((uint32_t)udpbuff[6] << 8) | (uint32_t)udpbuff[7];
                contentLenA = ((uint32_t)udpbuff[8] << 24) | ((uint32_t)udpbuff[9] << 16) | ((uint32_t)udpbuff[10] << 8) | (uint32_t)udpbuff[11];
                contentLenB = ((uint32_t)udpbuff[12] << 24) | ((uint32_t)udpbuff[13] << 16) | ((uint32_t)udpbuff[14] << 8) | (uint32_t)udpbuff[15];
                contentLenC = frameSize - contentLenA - contentLenB;
                packetCount = udpbuff[16] * 256 + udpbuff[17];
                packetStandardLen = udpbuff[20] * 256 + udpbuff[21];
                checkSum = udpbuff[24] * 256 + udpbuff[25];
                for(uint8_t i = 0; i < sizeof(packetState); i++){
                    packetState[i] = 0;
                }
                rFinish = 0;
            }
            packetId = udpbuff[18] * 256 + udpbuff[19]; //当前数据包号
            //将数据包加入缓存
            if(packetId < packetCount) {
                if(frameSize > sizeof(dataBuff) - packetHeaderLen)
                    return 0;//数据包过大
                //标准包或最后一包
                if((packetStandardLen == packetLen) || (packetId == (packetCount - 1))){
                    memcpy(dataBuff + packetId * (packetStandardLen - packetHeaderLen), udpbuff + packetHeaderLen, packetLen - packetHeaderLen);//复制数据包到总缓存
                    packetState[packetId / 8] |= (0x80 >> (packetId % 8)); //记录已接收的包
                }else{
                    return 0;
                }
            }
            //判断接收结束
            if((packetId == (packetCount - 1)) || (packetId == 0xffff)){
                //判断数据包是否接收完整
                if(packetId == 0xffff){
                    //System.out.println("结束包");
                }
                uint8_t lack = 0;
                resendQuantity = 0;
                for(uint16_t pn=0;pn<packetCount;pn++) {
                    if((packetState[pn/8] & (0x80 >> (pn % 8))) == 0) {
                        //System.out.println("frameId:" + frameId + " Packet missing: (id)" + pn);
                        resendQuantity += 1;
                        lack = 1;//未接收完整
                    }
                }
                if(lack){
                    return 2;
                }else{
                    uint16_t crc2 = 0;
                    for(uint32_t ln = 0; ln < frameSize; ln++){
                        crc2 += (dataBuff[ln]&0x0ff);
                    }
                    if(crc2 == checkSum){
                        rFinish = 1;
                        //System.out.println("Verification successful!" + " packetLen:" + packetLen);
                        return 3;
                    }else{
                        for(uint16_t i = 0; i < sizeof(packetState); i++){
                            packetState[i] = 0;
                        }
                        //System.out.println("Validation failed!" + (crc2&0xffffffff) + "," + (checkSum&0xffffffff) + ", frameSize:" + frameSize);
                        resendQuantity = packetCount;
                        return 2;
                    }
                }
            }
        }else{
            packetId = udpbuff[18] * 256 + udpbuff[19]; //当前数据包号
            return 0;
        }
    }else{
        //printf("return-3\n");
        return 0;
    }
    return 1;
}


//将要发送的数据封装成数据包返回，参数为数据帧号、数据类型1=普通消息，2=回应消息, 3=搜索设备, 4=配对、消息主体，消息长度，打包好的数据地址和长度
uint16_t generatingPackets(uint32_t msgId, uint8_t msgType, uint8_t *sdata, uint16_t slen, uint8_t **packet){
    tx_Buff[0] = msgId >> 24;
    tx_Buff[1] = msgId >> 16;
    tx_Buff[2] = msgId >> 8;
    tx_Buff[3] = msgId;
    tx_Buff[4] = msgType;
    uint16_t len = slen + 9;
    tx_Buff[5] = len >> 8;
    tx_Buff[6] = len;
    tx_Buff[7] = 0;
    tx_Buff[8] = 0;
    uint16_t dsum = 0;
    for(uint16_t i = 9; i < len; i++){
        tx_Buff[i] = sdata[i - 9];
        dsum += tx_Buff[i];
    }
    for(int i = 0; i < 7; i++){
        dsum += tx_Buff[i];
    }
    tx_Buff[7] = dsum >> 8;
    tx_Buff[8] = dsum;

    *packet = tx_Buff;
    uint32_t addr = (uint32_t)packet;//无作用，防止GCC报错
    return len;
}



//解析数据包
uint8_t* parsingPackets(uint8_t *buf, uint16_t len, uint32_t *msg_id, uint8_t *msg_type){
    uint16_t dsum = 0;
    uint16_t packet_len = 0; //数据包长度
    int16_t data_len = 0; //有效数据长度（去掉数据头）

    static uint8_t data_buff[256] = {0}; //开辟数据缓存256字节

    if(len >= 8){
        for(uint16_t i = 0; i < 7; i++){
            dsum += buf[i];
        }
        for(uint16_t i = 9; i < len; i++){
            dsum += buf[i];
        }
        if(buf[7] * 256 + buf[8] == dsum){
            *msg_id = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
            *msg_type = buf[4];
            packet_len = buf[5] * 256 + buf[6];
            data_len = packet_len - 9;
            if(packet_len != len){
                ESP_LOGE(TAG,"Packet length error: %u, %u\r\n", packet_len, len);
            }
            for(uint16_t i = 9; i < len; i++){
                data_buff[i-9] = buf[i];
            }
            data_buff[len] = 0;
        }else{
            ESP_LOGE(TAG,"Packet verification error: %u, %u\r\n", dsum, buf[7] * 256 + buf[8]);
            return (uint8_t*)NULL;
        }
        
    }else{
        ESP_LOGE(TAG,"The data is too short: %u\r\n", len);
        return (uint8_t*)NULL;
    }
    return data_buff;
}

//组合数据包到缓存，返回数据包长度
uint32_t dataJoin(uint8_t* udpSf,uint32_t f_id, uint8_t* buff1, uint32_t len1, uint8_t* buff2, uint32_t len2, uint8_t* buff3, uint32_t len3){
    uint8_t *pUSB = udpSf;
    //帧号
    frame_id = f_id;
    pUSB[0]=(uint8_t)(frame_id>>24);
    pUSB[1]=(uint8_t)(frame_id>>16);
    pUSB[2]=(uint8_t)(frame_id>>8);
    pUSB[3]=(uint8_t)(frame_id);
    //帧尺寸
    uint32_t frame_size = len1 + len2 + len3;
    pUSB[4]=(uint8_t)(frame_size>>24);
    pUSB[5]=(uint8_t)(frame_size>>16);
    pUSB[6]=(uint8_t)(frame_size>>8);
    pUSB[7]=(uint8_t)(frame_size);
    //第一部分数据长度
    pUSB[8]=(uint8_t)(len1>>24);
    pUSB[9]=(uint8_t)(len1>>16);
    pUSB[10]=(uint8_t)(len1>>8);
    pUSB[11]=(uint8_t)(len1);
    //第二部分数据长度
    pUSB[12]=(uint8_t)(len2>>24);
    pUSB[13]=(uint8_t)(len2>>16);
    pUSB[14]=(uint8_t)(len2>>8);
    pUSB[15]=(uint8_t)(len2);
    //分包数量
    uint16_t packet_data_len = PACKET_STANDARD_LEN - PACKET_HEADER_LEN;
    packet_count = frame_size / packet_data_len + ((frame_size%packet_data_len)==0?0:1);
    pUSB[16]=(uint8_t)(packet_count>>8);
    pUSB[17]=(uint8_t)(packet_count);
    //当前包号
    pUSB[18]=(uint8_t)0;
    pUSB[19]=(uint8_t)0;
    //标准包长度
    pUSB[20]=(uint8_t)(PACKET_STANDARD_LEN>>8);
    pUSB[21]=(uint8_t)(PACKET_STANDARD_LEN);
    //当前包长度
    pUSB[22]=(uint8_t)(PACKET_STANDARD_LEN>>8);
    pUSB[23]=(uint8_t)(PACKET_STANDARD_LEN);
    //累加校验先清0
    pUSB[24]=(uint8_t)0;
    pUSB[25]=(uint8_t)0;
    
    pUSB[26]=(uint8_t)0;
    pUSB[27]=(uint8_t)0;
    
    //复制第一部分数据
    uint16_t dsum = 0;
    uint8_t *pb = pUSB + PACKET_HEADER_LEN;
    uint8_t *pe = buff1 + len1;
    for(uint8_t *pn = buff1; pn < pe; pn ++){
        *pb = *pn;
        dsum += *pn;
        pb ++;
    }
    //复制第二部分数据
    pe = buff2 + len2;
    for(uint8_t *pn = buff2; pn < pe; pn ++){
        *pb = *pn;
        dsum += *pn;
        pb ++;
    }
    //复制第三部分数据
    pe = buff3 + len3;
    for(uint8_t *pn = buff3; pn < pe; pn ++){
        *pb = *pn;
        dsum += *pn;
        pb ++;
    }
    //赋校验值
    pUSB[24]=(uint8_t)(dsum>>8);
    pUSB[25]=(uint8_t)(dsum);
    
    //----------以下部分作为测试输出-----------
    //检测校验值是否正确
    /*dsum = 0;
    pe = pUSB + PACKET_HEADER_LEN + frame_size;
    for(uint8_t *pn = pUSB + PACKET_HEADER_LEN; pn < pe; pn ++){
        dsum += *pn;
    }
    if(dsum == pUSB[24] * 256 + pUSB[25]){
        printf("Verification successful! %u\r\n", dsum);
    }else{
        printf("Verification failed! %u, %u\r\n", dsum, pUSB[24] * 256 + pUSB[25]);
    }
    printf("join: frame_id = %u, frame_size = %u, packet_count = %u, packet_data_len = %u\r\n", frame_id, frame_size, packet_count, packet_data_len);
    */
    //--------------------------------------
    
    return frame_size + PACKET_HEADER_LEN;
}

//发送缓存数据
uint8_t udpSendBuffData(uint8_t* udpSf, int sock, struct sockaddr_storage source_addr){
    uint8_t *pUSB = udpSf;
    
    //取出数据头
    for(uint8_t i = 0; i < PACKET_HEADER_LEN; i ++){
        tx_buffer[i] = pUSB[i];
    }
    uint32_t frame_id = ((uint32_t)pUSB[0] << 24) | ((uint32_t)pUSB[1] << 16) | ((uint32_t)pUSB[2] << 8) | ((uint32_t)pUSB[3]);
    uint32_t frame_size = ((uint32_t)pUSB[4] << 24) | ((uint32_t)pUSB[5] << 16) | ((uint32_t)pUSB[6] << 8) | ((uint32_t)pUSB[7]);
    uint16_t packet_count = ((uint16_t)pUSB[16] << 8) | ((uint16_t)pUSB[17]);
    uint16_t packet_standard_len = ((uint16_t)pUSB[20] << 8) | ((uint16_t)pUSB[21]);
    
    //排除空包
    if((frame_size==0)||(packet_count==0)||(packet_standard_len==0)){
        printf("Packet format error 1!\r\n");
        return 0;
    }
    
    //循环发送数据包
    for(uint16_t pk = 0; pk < packet_count; pk ++){
        //本包包号
        tx_buffer[18] = pk >> 8;
        tx_buffer[19] = pk;
        //本包长度
        uint16_t packet_len = packet_standard_len;
        uint16_t packet_data_len = packet_standard_len - PACKET_HEADER_LEN;
        if((pk == packet_count - 1) && (frame_size % packet_data_len != 0)){//最后一包且不是标准包长
            packet_len = frame_size % packet_data_len + PACKET_HEADER_LEN;
            if(packet_len >= packet_standard_len){
                printf("Packet format error 2!\r\n");
                return 0;
            }
        }
        tx_buffer[22] = packet_len >> 8;
        tx_buffer[23] = packet_len;
        //Serial.printf("tx_buffer[22] = %u, tx_buffer[23] = %u\r\n", tx_buffer[22], tx_buffer[23]);
        //载入包内容
        uint8_t *pb = tx_buffer + PACKET_HEADER_LEN;
        uint8_t *pe = (pUSB + PACKET_HEADER_LEN) + pk * packet_data_len + (packet_len - PACKET_HEADER_LEN);
        for(uint8_t *pn = pUSB + PACKET_HEADER_LEN + pk * packet_data_len; pn < pe; pn ++){
            *pb = *pn;
            pb++;
        }
        //添加包校验
        tx_buffer[26] = 0;
        tx_buffer[27] = 0;
        uint16_t dsum = 0;
        pe = tx_buffer + packet_len;
        for(uint8_t *pn = tx_buffer; pn < pe; pn ++){
            dsum += *pn;
        }
        tx_buffer[26] = dsum >> 8;
        tx_buffer[27] = dsum;

        //发送
        int err = sendto(sock, tx_buffer, packet_len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
        if (err < 0) {
            //ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            ESP_LOGE(TAG, "Error occurred during sending: errno-1");
        }
        //----------以下部分作为测试输出-----------
        //if(pk == 0)
        //    printf("Send1: frame_id = %u, frame_size = %u, packet_count = %u, packet_len = %u\r\n", frame_id, frame_size, packet_count, packet_len);
        //--------------------------------------
    }
    return 1;   
}

//发送缓存数据指定数据包
uint8_t udpSendBuffDataPacket(uint8_t* udpSf, int sock, struct sockaddr_storage source_addr,uint16_t packet){
    uint8_t *pUSB = udpSf;

    //取出数据头
    for(uint8_t i = 0; i < PACKET_HEADER_LEN; i ++){
        tx_buffer[i] = pUSB[i];
    }
    uint32_t frame_id = ((uint32_t)pUSB[0] << 24) | ((uint32_t)pUSB[1] << 16) | ((uint32_t)pUSB[2] << 8) | ((uint32_t)pUSB[3]);
    uint32_t frame_size = ((uint32_t)pUSB[4] << 24) | ((uint32_t)pUSB[5] << 16) | ((uint32_t)pUSB[6] << 8) | ((uint32_t)pUSB[7]);
    uint16_t packet_count = ((uint16_t)pUSB[16] << 8) | ((uint16_t)pUSB[17]);
    uint16_t packet_standard_len = ((uint16_t)pUSB[20] << 8) | ((uint16_t)pUSB[21]);
    
    //排除空包
    if((frame_size==0)||(packet_count==0)||(packet_standard_len==0)){
        printf("Packet format error 1!\r\n");
        return 0;
    }
    
    //发送指定数据包
    uint16_t pk = packet;
    
    if((pk < packet_count) || (pk == 0xffff)){
        //本包包号
        tx_buffer[18] = pk >> 8;
        tx_buffer[19] = pk;
        //本包长度
        uint16_t packet_len = packet_standard_len;
        if(pk < packet_count){
            uint16_t packet_data_len = packet_standard_len - PACKET_HEADER_LEN;
            if((pk == packet_count - 1) && (frame_size % packet_data_len != 0)){//最后一包且不是标准包长
                packet_len = frame_size % packet_data_len + PACKET_HEADER_LEN;
                if(packet_len >= packet_standard_len){
                    printf("Packet format error 2!\r\n");
                    return 0;
                }
            }
            tx_buffer[22] = packet_len >> 8;
            tx_buffer[23] = packet_len;
            //载入包内容
            uint8_t *pb = tx_buffer + PACKET_HEADER_LEN;
            uint8_t *pe = (pUSB + PACKET_HEADER_LEN) + pk * packet_data_len + (packet_len - PACKET_HEADER_LEN);
            for(uint8_t *pn = pUSB + PACKET_HEADER_LEN + pk * packet_data_len; pn < pe; pn ++){
                *pb = *pn;
                pb++;
            }
        }else{
            packet_len = PACKET_HEADER_LEN;
            tx_buffer[22] = packet_len >> 8;
            tx_buffer[23] = packet_len;
        }
        
        //添加包校验
        tx_buffer[26] = 0;
        tx_buffer[27] = 0;
        uint16_t dsum = 0;
        uint8_t *pe = tx_buffer + packet_len;
        for(uint8_t *pn = tx_buffer; pn < pe; pn ++){
            dsum += *pn;
        }
        tx_buffer[26] = dsum >> 8;
        tx_buffer[27] = dsum;

        //发送
        int err = sendto(sock, tx_buffer, packet_len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
        if (err < 0) {
            //ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            ESP_LOGE(TAG, "Error occurred during sending: errno-2");
        }
    }
    return 1;   
}

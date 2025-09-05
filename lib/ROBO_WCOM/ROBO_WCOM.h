#ifndef ROBO_WCOM_H
#define ROBO_WCOM_H

#include <Arduino.h>

namespace ROBO_WCOM
{
    constexpr size_t CARRIED_DATA_MAX_SIZE = 200;
    constexpr size_t RECEIVE_BUFFER_SIZE   = 64;

    struct __attribute__((packed)) PacketData {
        uint32_t timestamp;                         // 受信時刻（millis()）
        uint8_t  address[6];                        // 送信元アドレス
        uint8_t  carriedSize;                       // データサイズ
        uint8_t  carriedData[CARRIED_DATA_MAX_SIZE];// データ本体
    };

    // API
    int Init(const uint8_t sourceAddress[6], const uint8_t distAddress[6], uint32_t timeoutMS);
    int SendPacket(uint32_t timestamp, const uint8_t* data, uint8_t size);
    int ReceivePacket(uint32_t nowMillis, int32_t* timestamp, uint8_t* address, uint8_t* data, uint8_t* size);
    int16_t ReceivedCapacity();
}

#endif

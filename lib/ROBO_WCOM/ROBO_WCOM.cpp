#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#define __GLOBAL_DEFINE__
#include "ROBO_WCOM.h"

typedef struct  __attribute__((packed)) {
    rPacketData_t datas;
    uint32_t crc32;            // CRC32
} rPacket_t;

static rPacket_t receivedPacketBuffer[RECEIVE_BUFFER_SIZE];
static volatile uint16_t receiveBufferHead = 0;
static volatile uint16_t receiveBufferTail = 0;
static volatile int16_t receivedCount = 0;

static rPacket_t sendPacket;
static uint8_t ownAddress[6];
static uint8_t peerAddress[6];
static esp_now_peer_info_t peerInfo;
static uint32_t lastReceivedMillis = 0;
static uint32_t timeoutMillis;


static void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
static uint32_t ROBO_WCOM_calcCRC32(const rPacketData_t &data);

int ROBO_WCOM_Init(uint8_t sourceAddress[6], uint8_t distAddress[6], uint32_t timeoutMS)
{
    rPacket_t zDummy;
    int i;

    timeoutMillis = timeoutMS;
    lastReceivedMillis = 0;

    receiveBufferHead = 0;
    receiveBufferTail = 0;
    receivedCount = 0;

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        return -1;
    }
    memcpy(ownAddress, sourceAddress, 6);
    memcpy(peerAddress, distAddress, 6);
    memset(&peerInfo, 0, sizeof(peerInfo));  // ← これを追加
    memcpy(peerInfo.peer_addr, distAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        return -2;
    }
    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);
    return 0;
}

int ROBO_WCOM_SendPacket(uint32_t timestamp, const uint8_t* data, uint8_t size)
{
    sendPacket.datas.timestamp = timestamp;
    memcpy(sendPacket.datas.address, ownAddress, 6);
    if (size > CARRIED_DATA_MAX_SIZE)
    {
        size = CARRIED_DATA_MAX_SIZE;
    }
    sendPacket.datas.carriedSize = size;
    memcpy(sendPacket.datas.carriedData, data, size);
    sendPacket.crc32 = ROBO_WCOM_calcCRC32(sendPacket.datas);
    // if (random(0,10) == 0)
    // {
    //     // 10%の確率でデータを壊す
    //     sendPacket.crc32 ^= 0xFFFFFFFF;
    // }
    if (esp_now_send(peerAddress, (uint8_t *) &sendPacket, sizeof(rPacket_t)) != ESP_OK)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int ROBO_WCOM_ReceivePacket(uint32_t nowMills, int32_t* timestamp, uint8_t *address, uint8_t* data, uint8_t* size)
{
    uint32_t calcCRC = 0xFFFFFFFF;
    uint32_t readCRC = 0xFFFFFFFF;
    if (nowMills - lastReceivedMillis > timeoutMillis)
    {
        // タイムアウト
        return -1;
    }
    else
    {
        calcCRC =  ROBO_WCOM_calcCRC32(receivedPacketBuffer[receiveBufferTail].datas);
        *timestamp = receivedPacketBuffer[receiveBufferTail].datas.timestamp;
        memcpy(address, receivedPacketBuffer[receiveBufferTail].datas.address, 6);
        *size = receivedPacketBuffer[receiveBufferTail].datas.carriedSize;
        memcpy(data, receivedPacketBuffer[receiveBufferTail].datas.carriedData, *size);
        readCRC = receivedPacketBuffer[receiveBufferTail].crc32;

        receivedCount--;
        receiveBufferTail = (receiveBufferTail + 1) % RECEIVE_BUFFER_SIZE;

        if (readCRC != calcCRC)
        {
            return -2;
        }
        else
        {
            // CRCチェックOK タイムアウト無し
            return 0;
        }

    }
}

static uint32_t ROBO_WCOM_calcCRC32(const rPacketData_t &data)
{
    uint32_t crc = 0xFFFFFFFF;
    int i, j;
    uint8_t *byteData = (uint8_t*)&data;
    for (i = 0; i < sizeof(rPacketData_t); i++) {
        crc ^= byteData[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFF;
}

int16_t ROBO_WCOM_ReceivedCapacity(void)
{
    return receivedCount;
}

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
    if (len != sizeof(rPacket_t))
    {
        // サイズエラー
        return;
    }
    else
    {
        lastReceivedMillis = millis();
        memcpy(&receivedPacketBuffer[receiveBufferHead], incomingData, sizeof(rPacket_t));
        receivedCount++;
        receiveBufferHead = (receiveBufferHead + 1) % RECEIVE_BUFFER_SIZE;
        return ;
    }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // 送信完了コールバック（今のところ何もしない）
    return;
}

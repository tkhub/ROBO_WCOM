#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#define __GLOBAL_DEFINE__
#include "ROBO_WCOM.h"

typedef struct  __attribute__((packed)) {
    rPacketData_t datas;
    uint32_t crc32;            // CRC32
} rPacket_t;

static rPacket_t receivedPacket;
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
    timeoutMillis = timeoutMS;
    receivedPacket.datas.timestamp = 0;
    receivedPacket.datas.carriedSize = 0;
    memset(receivedPacket.datas.address, 0, 6);
    memset(receivedPacket.datas.carriedData, 0, CARRIED_DATA_MAX_SIZE);
    receivedPacket.crc32 = 0;
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
    uint32_t calcCRC =  ROBO_WCOM_calcCRC32(receivedPacket.datas);
    *timestamp = receivedPacket.datas.timestamp;
    memcpy(address, receivedPacket.datas.address, 6);
    *size = receivedPacket.datas.carriedSize;
    memcpy(data, receivedPacket.datas.carriedData, *size);
    // Serial.printf(" calcCRC=0x%08X, rcvCRC=0x%08X", calcCRC, receivedPacket.crc32);
    if (nowMills - lastReceivedMillis > timeoutMillis)
    {
        // タイムアウト
        return -1;
    }
    else if (receivedPacket.crc32 != calcCRC)
    {
        return -2;
    }
    else
    {
        // CRCチェックOK タイムアウト無し
        return 0;
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

bool ROBO_WCOM_isAvailableReceivePacket(void)
{
    return true;
}

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
    lastReceivedMillis = millis();
    memcpy(&receivedPacket, incomingData, sizeof(rPacket_t));
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // 送信完了コールバック（今のところ何もしない）
    return;
}

#include "ROBO_WCOM.h"
#include <WiFi.h>
#include <esp_now.h>
#include <cstring>

namespace ROBO_WCOM
{
    struct __attribute__((packed)) Packet {
        PacketData data;
        uint32_t crc32;
    };

    static Packet recvBuffer[RECEIVE_BUFFER_SIZE];
    static volatile uint16_t head = 0;
    static volatile uint16_t tail = 0;
    static volatile int16_t  count = 0;

    static Packet sendPacket;
    static uint8_t ownAddr[6];
    static uint8_t peerAddr[6];
    static esp_now_peer_info_t peerInfo{};
    static uint32_t lastRecvMillis = 0;
    static uint32_t timeoutMillis  = 0;

    // --- 内部関数 ---
    static uint32_t calcCRC32(const PacketData& data)
    {
        uint32_t crc = 0xFFFFFFFF;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
        for (size_t i = 0; i < sizeof(PacketData); ++i) {
            crc ^= bytes[i];
            for (int j = 0; j < 8; ++j)
                crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
        }
        return crc ^ 0xFFFFFFFF;
    }

    static void pushToBuffer(const Packet& pkt)
    {
        recvBuffer[head] = pkt;
        head = (head + 1) % RECEIVE_BUFFER_SIZE;
        if (count < RECEIVE_BUFFER_SIZE) {
            ++count;
        } else {
            // 上書き時はtailも進める
            tail = (tail + 1) % RECEIVE_BUFFER_SIZE;
        }
    }

    static bool popFromBuffer(Packet& pkt)
    {
        if (count <= 0) return false;
        pkt = recvBuffer[tail];
        tail = (tail + 1) % RECEIVE_BUFFER_SIZE;
        --count;
        return true;
    }

    static void onDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len)
    {
        if (len != sizeof(Packet)) return;
        lastRecvMillis = millis();
        Packet pkt;
        memcpy(&pkt, incomingData, sizeof(Packet));
        pushToBuffer(pkt);
    }

    static void onDataSent(const uint8_t*, esp_now_send_status_t) {}

    // --- 公開API ---
    int Init(const uint8_t sourceAddress[6], const uint8_t distAddress[6], uint32_t timeoutMS)
    {
        timeoutMillis = timeoutMS;
        lastRecvMillis = 0;
        head = tail = count = 0;

        memcpy(ownAddr, sourceAddress, sizeof(ownAddr));
        memcpy(peerAddr, distAddress, sizeof(peerAddr));

        WiFi.mode(WIFI_STA);
        if (esp_now_init() != ESP_OK) return -1;

        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, distAddress, sizeof(peerAddr));
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) return -2;

        esp_now_register_recv_cb(onDataRecv);
        esp_now_register_send_cb(onDataSent);
        return 0;
    }

    int SendPacket(uint32_t timestamp, const uint8_t* data, uint8_t size)
    {
        sendPacket.data.timestamp = timestamp;
        memcpy(sendPacket.data.address, ownAddr, sizeof(ownAddr));

        if (size > CARRIED_DATA_MAX_SIZE) size = CARRIED_DATA_MAX_SIZE;
        sendPacket.data.carriedSize = size;
        memcpy(sendPacket.data.carriedData, data, size);

        sendPacket.crc32 = calcCRC32(sendPacket.data);

        return (esp_now_send(peerAddr, reinterpret_cast<uint8_t*>(&sendPacket), sizeof(Packet)) == ESP_OK) ? 0 : -1;
    }

    int ReceivePacket(uint32_t nowMillis, int32_t* timestamp, uint8_t* address, uint8_t* data, uint8_t* size)
    {
        if (nowMillis - lastRecvMillis > timeoutMillis) return -1;

        Packet pkt;
        if (!popFromBuffer(pkt)) return -3; // バッファ空

        if (calcCRC32(pkt.data) != pkt.crc32) return -2;

        *timestamp = pkt.data.timestamp;
        memcpy(address, pkt.data.address, sizeof(pkt.data.address));
        *size = pkt.data.carriedSize;
        memcpy(data, pkt.data.carriedData, *size);

        return 0;
    }

    int16_t ReceivedCapacity()
    {
        return count;
    }
}

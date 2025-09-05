#include "ROBO_WCOM.h"
#include <WiFi.h>
#include <esp_now.h>
#include <cstring>

namespace ROBO_WCOM
{
    /**
     * @brief 送受信パケット構造（CRC付き）
     */
    struct __attribute__((packed)) Packet {
        PacketData data; ///< データ部
        uint32_t crc32;  ///< CRC32値
    };

    //=== 内部状態 ===//
    static Packet recvBuffer[RECEIVE_BUFFER_SIZE]; ///< 受信リングバッファ
    static volatile uint16_t head = 0;             ///< バッファ書き込み位置
    static volatile uint16_t tail = 0;             ///< バッファ読み出し位置
    static volatile int16_t  count = 0;            ///< バッファ内パケット数

    static Packet sendPacket;                      ///< 送信用パケット
    static uint8_t ownAddr[6];                     ///< 自デバイスMAC
    static uint8_t peerAddr[6];                    ///< 通信相手MAC
    static esp_now_peer_info_t peerInfo{};         ///< ESP-NOW Peer情報
    static uint32_t lastRecvMillis = 0;            ///< 最終受信時刻
    static uint32_t timeoutMillis  = 0;            ///< タイムアウト時間

    //=== 内部関数プロトタイプ ===//
    static uint32_t calcCRC32(const PacketData& data);
    static void pushToBuffer(const Packet& pkt);
    static bool popFromBuffer(Packet& pkt);
    static void onDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len);
    static void onDataSent(const uint8_t*, esp_now_send_status_t);

    //=== 内部関数実装 ===//

    /**
     * @brief CRC32計算
     * @param data パケットデータ部
     * @return CRC32値
     */
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

    /**
     * @brief 受信バッファにパケットを追加
     * @param pkt 追加するパケット
     */
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

    /**
     * @brief 受信バッファからパケットを取り出す
     * @param pkt 取り出したパケット格納先
     * @return true:成功 / false:空
     */
    static bool popFromBuffer(Packet& pkt)
    {
        if (count <= 0) return false;
        pkt = recvBuffer[tail];
        tail = (tail + 1) % RECEIVE_BUFFER_SIZE;
        --count;
        return true;
    }

    /**
     * @brief ESP-NOW受信コールバック
     * @param mac 送信元MACアドレス
     * @param incomingData 受信データ
     * @param len データ長
     */
    static void onDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len)
    {
        if (len != sizeof(Packet)) return;
        lastRecvMillis = millis();
        Packet pkt;
        memcpy(&pkt, incomingData, sizeof(Packet));
        pushToBuffer(pkt);
    }

    /**
     * @brief ESP-NOW送信完了コールバック
     * @param mac_addr 送信先MAC
     * @param status   送信ステータス
     */
    static void onDataSent(const uint8_t*, esp_now_send_status_t) {}

    //=== 公開API実装 ===//

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

    /**
     * @brief 送信処理
     * @param   timestamp   送信時刻（任意の基準でOK）
     * @param   data        送信データへのポインタ
     * @param   size        送信データサイズ（最大 CARRIED_DATA_MAX_SIZE）
     * @return              0:成功 / -1:送信失敗
     */
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

    /**
     * @brief 受信処理。バッファから1パケット取り出す。
     *
     * @param nowMillis 現在時刻(ms)
     * @param timestamp 受信パケットのタイムスタンプ格納先
     * @param address 送信元アドレス格納先（6バイト）
     * @param data 受信データ格納先
     * @param size 受信データサイズ格納先
     * @return * int           0:成功 / -1:タイムアウト / -2:CRCエラー / -3:バッファ空
     */
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

    /**
     * @brief 受信バッファ内のパケット数を取得
     *
     * @return int16_t バッファ内のパケット数
     */
    int16_t ReceivedCapacity()
    {
        return count;
    }
}



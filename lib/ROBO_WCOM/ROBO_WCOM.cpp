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
    static Status extractPacketData(const Packet& pkt, uint32_t* timestamp, uint8_t* address, uint8_t* data, uint8_t* size);

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
    static void onDataSent(const uint8_t*, esp_now_send_status_t) 
    {
        // 送信完了時の処理（必要なら実装）
        // 今回は特に何もしない
        return;
    }


    /**
     * @brief パケットからデータを抽出（CRCチェック付き）
     * @param pkt     対象パケット
     * @param timestamp タイムスタンプ格納先
     * @param address   アドレス格納先（6バイト）
     * @param data      データ本体格納先
     * @param size      データサイズ格納先
     * @return Status::Ok / Status::CrcError / Status::InvalidArg
     */
    static Status extractPacketData(const Packet& pkt, uint32_t* timestamp, uint8_t* address, uint8_t* data, uint8_t* size)
    {

        if (calcCRC32(pkt.data) != pkt.crc32)
        {
            return Status::CrcError;
        }

        *timestamp = pkt.data.timestamp;
        memcpy(address, pkt.data.address, sizeof(pkt.data.address));
        *size = pkt.data.carriedSize;
        memcpy(data, pkt.data.carriedData, *size);
        return Status::Ok;
    }


    //=== 公開API実装 ===//

    /**
     * @brief 通信初期化
     * @param sourceAddress 自デバイスのMACアドレス（6バイト）
     * @param distAddress   通信相手のMACアドレス（6バイト）
     * @param nowMillis     受信タイムアウト時間（ミリ秒）
     * @param timeoutMS     受信タイムアウト時間（ミリ秒）
     * @return ステータスコード (Status)
     */
    Status Init(const uint8_t sourceAddress[6], const uint8_t distAddress[6], uint32_t nowMillis, uint32_t timeoutMS)
    {
        uint8_t i;
        Packet zeroPkt;
        timeoutMillis = timeoutMS;
        lastRecvMillis = nowMillis;
        head = 0;
        tail = 0;
        count = 0;

        zeroPkt.data.timestamp = 0;
        memset(zeroPkt.data.address, 0, sizeof(zeroPkt.data.address));
        zeroPkt.data.carriedSize = 0;
        memset(zeroPkt.data.carriedData, 0, sizeof(zeroPkt.data.carriedData));
        zeroPkt.crc32 = calcCRC32(zeroPkt.data);
        // バッファ初期化

        for (i = 0; i < RECEIVE_BUFFER_SIZE; i++)
        {
            pushToBuffer(zeroPkt);
        }
        FlushBuffer();

        memcpy(ownAddr, sourceAddress, sizeof(ownAddr));
        memcpy(peerAddr, distAddress, sizeof(peerAddr));

        WiFi.mode(WIFI_STA);
        if (esp_now_init() != ESP_OK)
        {
            return Status::EspNowInitFail;
        }

        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, distAddress, sizeof(peerAddr));
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            return Status::AddPeerFail;
        }

        esp_now_register_recv_cb(onDataRecv);
        esp_now_register_send_cb(onDataSent);
        return Status::Ok;
    }

    /**
     * @brief パケット送信
     * @param timestamp 送信時刻（任意の基準でOK）
     * @param data      送信データへのポインタ
     * @param size      送信データサイズ（最大 CARRIED_DATA_MAX_SIZE）
     * @return ステータスコード (Status)
     */
    Status SendPacket(uint32_t timestamp, const uint8_t* data, uint8_t size)
    {
        sendPacket.data.timestamp = timestamp;
        memcpy(sendPacket.data.address, ownAddr, sizeof(ownAddr));

        if (size > CARRIED_DATA_MAX_SIZE)
        {
            size = CARRIED_DATA_MAX_SIZE;
        }
        sendPacket.data.carriedSize = size;
        memcpy(sendPacket.data.carriedData, data, size);

        sendPacket.crc32 = calcCRC32(sendPacket.data);

        if (esp_now_send(peerAddr, reinterpret_cast<uint8_t*>(&sendPacket), sizeof(Packet)) == ESP_OK)
        {
            return Status::Ok;
        }
        else
        {
            return Status::SendFail;
        }
    }

    /**
     * @brief パケット受信
     * @param nowMillis 現在時刻（millis）
     * @param timestamp 受信パケットのタイムスタンプ格納先
     * @param address   送信元アドレス格納先（6バイト）
     * @param data      受信データ格納先
     * @param size      受信データサイズ格納先
     * @return ステータスコード (Status)
     */
    Status PopOldestPacket(uint32_t nowMillis, uint32_t* timestamp, uint8_t* address, uint8_t* data, uint8_t* size)
    {
        Packet pkt;
        Status pktStatus;
        if (nowMillis - lastRecvMillis > timeoutMillis)
        {
            Packet zeroPkt;
            zeroPkt.data.timestamp = 0;
            memset(zeroPkt.data.address, 0, sizeof(zeroPkt.data.address));
            zeroPkt.data.carriedSize = 0;
            memset(zeroPkt.data.carriedData, 0, sizeof(zeroPkt.data.carriedData));
            zeroPkt.crc32 = calcCRC32(zeroPkt.data);
            extractPacketData(zeroPkt, timestamp, address, data, size);
            return Status::Timeout;
        }

        if (!popFromBuffer(pkt))
        {
            Packet zeroPkt;
            zeroPkt.data.timestamp = 0;
            memset(zeroPkt.data.address, 0, sizeof(zeroPkt.data.address));
            zeroPkt.data.carriedSize = 0;
            memset(zeroPkt.data.carriedData, 0, sizeof(zeroPkt.data.carriedData));
            zeroPkt.crc32 = calcCRC32(zeroPkt.data);
            extractPacketData(zeroPkt, timestamp, address, data, size);
            return Status::BufferEmpty;
        }
        pktStatus = extractPacketData(pkt, timestamp, address, data, size);
        return pktStatus;
    }

    /**
     * @brief バッファの最新データをチェックする。バッファは操作しない。
     * 
     * @param nowMillis 現在時刻（millis）
     * @param timestamp 受信パケットのタイムスタンプ格納先
     * @param address   送信元アドレス格納先（6バイト）
     * @param data      受信データ格納先
     * @param size      受信データサイズ格納先
     * @return ステータスコード (Status)
     */
    Status PeekLatestPacket(uint32_t nowMillis, uint32_t* timestamp, uint8_t* address, uint8_t* data, uint8_t* size)
    {
        Packet pkt;
        Status pktStatus;
        if (nowMillis - lastRecvMillis > timeoutMillis)
        {
            Packet zeroPkt;
            zeroPkt.data.timestamp = 0;
            memset(zeroPkt.data.address, 0, sizeof(zeroPkt.data.address));
            zeroPkt.data.carriedSize = 0;
            memset(zeroPkt.data.carriedData, 0, sizeof(zeroPkt.data.carriedData));
            zeroPkt.crc32 = calcCRC32(zeroPkt.data);
            extractPacketData(zeroPkt, timestamp, address, data, size);
            return Status::Timeout;
        }
        pkt = recvBuffer[tail];
        pktStatus = extractPacketData(pkt, timestamp, address, data, size);
        return pktStatus;
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

    /**
     * @brief バッファのクリア
     * 
     * @return Status 
     */
    Status FlushBuffer(void)
    {
        head = 0;
        tail = 0;
        count = 0;
        return Status::Ok;
    }

    /**
     * @brief ステータスコードを文字列に変換
     * @param s ステータスコード
     * @return 文字列
     */
    const char* ToString(Status s)
    {
        switch (s) {
            case Status::Ok:              return "Ok";
            case Status::Timeout:         return "Timeout";
            case Status::CrcError:        return "CRC error";
            case Status::BufferEmpty:     return "Buffer empty";
            case Status::InvalidArg:      return "Invalid argument";
            case Status::EspNowInitFail:  return "ESP-NOW init failed";
            case Status::AddPeerFail:     return "Add peer failed";
            case Status::SendFail:        return "Send failed";
            default:                      return "Unknown";
        }
    }
}

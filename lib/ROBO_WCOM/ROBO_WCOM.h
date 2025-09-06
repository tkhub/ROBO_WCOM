#ifndef ROBO_WCOM_H
#define ROBO_WCOM_H

#include <Arduino.h>

namespace ROBO_WCOM
{

    /**
     * @brief ライブラリ共通のステータス
     * @details
     * - 正常系は Ok のみ（0）
     * - 異常系は用途別に負の値を割当て
     */
    enum class Status : int8_t {
        Ok               = 0,

        // 共通
        Timeout          = -1,
        CrcError         = -2,
        BufferEmpty      = -3,
        InvalidArg       = -4,

        // 初期化
        EspNowInitFail   = -10,
        AddPeerFail      = -11,

        // 送信
        SendFail         = -20,
    };


    /**
     * @brief 搬送データの最大サイズ（バイト）
     */
    constexpr size_t CARRIED_DATA_MAX_SIZE = 200;

    /**
     * @brief 受信バッファの最大保持パケット数
     */
    constexpr size_t RECEIVE_BUFFER_SIZE   = 64;

    /**
     * @brief 通信パケットのデータ部構造
     * @details
     * - タイムスタンプ、送信元アドレス、データサイズ、データ本体を含む
     * - `__attribute__((packed))` によりパディングを排除
     */
    struct __attribute__((packed)) PacketData {
        uint32_t timestamp;                         ///< 受信時刻（millis()）
        uint8_t  address[6];                        ///< 送信元デバイスのMACアドレス
        uint8_t  carriedSize;                       ///< 搬送データサイズ
        uint8_t  carriedData[CARRIED_DATA_MAX_SIZE];///< 搬送データ本体
    };

    /**
     * @brief 通信初期化
     * @param sourceAddress 自デバイスのMACアドレス（6バイト）
     * @param distAddress   通信相手のMACアドレス（6バイト）
     * @param timeoutMS     受信タイムアウト時間（ミリ秒）
     * @return ステータスコード (Status)
     */
    Status Init(const uint8_t sourceAddress[6], const uint8_t distAddress[6], uint32_t timeoutMS);

    /**
     * @brief パケット送信
     * @param timestamp 送信時刻（任意の基準でOK）
     * @param data      送信データへのポインタ
     * @param size      送信データサイズ（最大 CARRIED_DATA_MAX_SIZE）
     * @return ステータスコード (Status)
     */
    Status SendPacket(uint32_t timestamp, const uint8_t* data, uint8_t size);

    /**
     * @brief パケット受信
     * @param nowMillis 現在時刻（millis）
     * @param timestamp 受信パケットのタイムスタンプ格納先
     * @param address   送信元アドレス格納先（6バイト）
     * @param data      受信データ格納先
     * @param size      受信データサイズ格納先
     * @return ステータスコード (Status)
     */
    Status PopOldestPacket(uint32_t nowMillis, uint32_t* timestamp, uint8_t* address, uint8_t* data, uint8_t* size);

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
    Status PeekLatestPacket(uint32_t nowMillis, uint32_t* timestamp, uint8_t* address, uint8_t* data, uint8_t* size);

    /**
     * @brief バッファのクリア
     * 
     * @return Status 
     */
    Status FlushBuffer(void);

    /**
     * @brief 受信バッファ内のパケット数を取得
     * @return バッファ内のパケット数
     */
    int16_t ReceivedCapacity(void);
    

    /**
     * @brief ステータスを人間可読な文字列へ変換
     */
    const char* ToString(Status s);
}

#endif /* ROBO_WCOM_H */

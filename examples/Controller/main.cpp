#include <Arduino.h>
#include "ROBO_WCOM.h"

#define TEST_STRING_SIZE 64  ///< 送信メッセージの最大長（バイト）

//---------------------------------------------
// 動作モード切り替え
//---------------------------------------------
#define BOARD_A 1  ///< 送信側ボード
#define BOARD_B 2  ///< 受信側ボード
// #define BOARD_IS BOARD_A
#define BOARD_IS BOARD_B

//---------------------------------------------
// 通信相手のMACアドレス設定
//---------------------------------------------
uint8_t MACADDRESS_BOARD_A[6] = {0x08, 0xB6, 0x1F, 0xEE, 0x42, 0xF4};
uint8_t MACADDRESS_BOARD_B[6] = {0xEC, 0xE3, 0x34, 0xD1, 0x36, 0xBC};

int16_t testCounter = 0;  ///< 送信カウンタ
ROBO_WCOM::PacketData receivedPacket; ///< 受信データ格納用

/**
 * @brief テスト送信関数
 * @param counter   送信カウンタ値
 * @param nowMillis 現在時刻（millis）
 * @param randomVal 送信データに含める乱数値
 */
void testSend(uint16_t counter, uint32_t nowMillis, uint32_t randomVal);

/**
 * @brief テスト受信関数
 * @param nowMillis 現在時刻（millis）
 */
void testReceive(uint32_t nowMillis);

//---------------------------------------------
// Arduino標準関数
//---------------------------------------------
/**
 * @brief 初期化処理
 * - シリアル通信開始
 * - ROBO_WCOM通信初期化
 */
void setup()
{
    Serial.begin(115200);
    Serial.println("Board Boot");

#if BOARD_IS == BOARD_A
    auto initStatus = ROBO_WCOM::Init(MACADDRESS_BOARD_A, MACADDRESS_BOARD_B, 1000);
    Serial.print("Controller communication started. : Status=");
    Serial.println(ROBO_WCOM::ToString(initStatus));
#else
    auto initStatus = ROBO_WCOM::Init(MACADDRESS_BOARD_B, MACADDRESS_BOARD_A, 1000);
    Serial.println("Robo communication started. : Status=");
    Serial.println(ROBO_WCOM::ToString(initStatus));
#endif
}

/**
 * @brief メインループ
 * - BOARD_A: 定期的にデータ送信
 * - BOARD_B: 受信バッファにデータがあれば受信処理
 */
void loop()
{
    uint32_t nowMillis = millis();
    int cap;

#if BOARD_IS == BOARD_A
    // 送信側：100回連続送信
    for (cap = 0; cap < 100; cap++)
    {
        uint32_t randomVal = random(0, 100);
        testSend(testCounter, nowMillis, randomVal);
        Serial.println();
        testCounter++;
        delay(1);
    }
    delay(2000);

#else
    // 受信側：バッファにデータがあれば受信処理
    cap = ROBO_WCOM::ReceivedCapacity();
    Serial.print("Capacity=");
    Serial.print(cap);
    testReceive(nowMillis);
    Serial.println();
    delay(10);
#endif
}

//---------------------------------------------
// ユーティリティ関数
//---------------------------------------------
void testSend(uint16_t counter, uint32_t nowMillis, uint32_t randomVal)
{
    char sendData[TEST_STRING_SIZE];
    snprintf(sendData, TEST_STRING_SIZE, "%d,\t%lu * %d =\t%lu",
             counter, randomVal, counter, randomVal * counter);

    // パケット送信
    ROBO_WCOM::SendPacket(nowMillis, reinterpret_cast<uint8_t*>(sendData), strlen(sendData) + 1);

    Serial.print(sendData);
}

void testReceive(uint32_t nowMillis)
{
    char receivedString[128];
    uint8_t size;
    uint32_t timestamp;
    uint8_t address[6];
    uint8_t data[ROBO_WCOM::CARRIED_DATA_MAX_SIZE];

    // パケット受信
    auto rcv_status = ROBO_WCOM::PopOldestPacket(nowMillis, &timestamp, address, data, &size);

    // ステータス表示
    switch (rcv_status)
    {
    case ROBO_WCOM::Status::Ok:
        Serial.print("\t || RECEIVE OK    ");
        break;
    case ROBO_WCOM::Status::Timeout:
        Serial.print("\t || TIMEOUT ERROR ");
        break;
    case ROBO_WCOM::Status::BufferEmpty:
        Serial.print("\t || BUFFER EMPTY  ");
        break;
    case ROBO_WCOM::Status::CrcError:
        Serial.print("\t || CRC ERROR     ");
        break;
    default:
        Serial.print("\t || UNKNOWN ERROR ");
        break;
    }

    // 受信内容表示
    snprintf(receivedString, sizeof(receivedString),
             "|| Received: time=%ld,\taddr=%02X:%02X:%02X:%02X:%02X:%02X, data=%s,\tsize=%d",
             timestamp,
             address[0], address[1], address[2],
             address[3], address[4], address[5],
             data, size);
    

    Serial.print(receivedString);
}

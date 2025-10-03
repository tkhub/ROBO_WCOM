#include <Arduino.h>
#include <ROBO_WCOM.h>
#include "MACADDRESS.h"
#define TEST_STRING_SIZE 64  ///< 送信メッセージの最大長（バイト）

//---------------------------------------------
// ボード切り替え
//---------------------------------------------
#define BOARD_A         1
#define BOARD_B         2
#define THIS_BOARD_IS   BOARD_A

//---------------------------------------------
// 動作モード切り替え
//---------------------------------------------
#define SEND_BOARD      3
#define RECEIVE_BOARD   4
#define MY_ROLL_IS      SEND_BOARD

#define BUFFERED_RECEIVE        5
#define NON_BUFFERED_RECEIVE    6
#define RECIVE_MODE_IS   NON_BUFFERED_RECEIVE


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
void setup()
{
    Serial.begin(115200);
    Serial.println("Board Boot");

#if THIS_BOARD_IS == BOARD_A
    auto initStatus = ROBO_WCOM::Init(MACADDRESS_BOARD_A, MACADDRESS_BOARD_B, millis(), 1000);
    Serial.print("Controller communication started. : Status=");
    Serial.println(ROBO_WCOM::ToString(initStatus));
#else
    auto initStatus = ROBO_WCOM::Init(MACADDRESS_BOARD_B, MACADDRESS_BOARD_A, millis(), 1000);
    Serial.println("Robo communication started. : Status=");
    Serial.println(ROBO_WCOM::ToString(initStatus));
#endif
}

void loop()
{
    uint32_t nowMillis = millis();
    uint32_t bTime, aTime;
    int cap;

#if MY_ROLL_IS == SEND_BOARD
    // 送信側：100回連続送信
    for (cap = 0; cap < 100; cap++)
    {
        uint32_t randomVal = random(0, 100);
        bTime = micros();
        testSend(testCounter, nowMillis, randomVal);
        aTime = micros();
        Serial.print(" : ");
        Serial.print(aTime-bTime);
        Serial.print(" : ");
        Serial.println();
        testCounter++;
        delay(10);
    }
    // delay(2000);

#else
    cap = ROBO_WCOM::ReceivedCapacity();
    Serial.print("t=");
    Serial.print(nowMillis);
    Serial.print(" Capacity=");
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

#if RECIVE_MODE_IS == BUFFERED_RECEIVE
    auto rcv_status = ROBO_WCOM::PopOldestPacket(nowMillis, &timestamp, address, data, &size);
#else
    auto rcv_status = ROBO_WCOM::PeekLatestPacket(nowMillis, &timestamp, address, data, &size);
#endif

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

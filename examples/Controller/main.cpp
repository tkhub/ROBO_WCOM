#include <Arduino.h>
#include "ROBO_WCOM.h"

#define TEST_STRING_SIZE 64

// 動作モード切り替え
#define BOARD_A 1
#define BOARD_B 2
#define BOARD_IS BOARD_A
// #define BOARD_IS BOARD_B

// MACアドレス設定
uint8_t MACADDRESS_BOARD_A[6] = {0x08, 0xB6, 0x1F, 0xEE, 0x42, 0xF4};
uint8_t MACADDRESS_BOARD_B[6] = {0xEC, 0xE3, 0x34, 0xD1, 0x36, 0xBC};

int16_t testCounter = 0;
ROBO_WCOM::PacketData receivedPacket;

void testSend(uint16_t counter, uint32_t nowMillis, uint32_t randomVal);
void testReceive(uint32_t nowMillis);

void setup()
{
    Serial.begin(115200);
    Serial.println("Board Boot");

#if BOARD_IS == BOARD_A
    Serial.println("Controller communication started.");
    Serial.println(ROBO_WCOM::Init(MACADDRESS_BOARD_A, MACADDRESS_BOARD_B, 1000));
#else
    Serial.println("Robo communication started.");
    Serial.println(ROBO_WCOM::Init(MACADDRESS_BOARD_B, MACADDRESS_BOARD_A, 1000));
#endif
}

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
    if (cap > 0)
    {
        Serial.print("Capacity=");
        Serial.print(cap);
        testReceive(nowMillis);
        Serial.println();
    }
    else
    {
        Serial.print("Capacity=");
        Serial.println(cap);
    }
    delay(10);
#endif
}

void testSend(uint16_t counter, uint32_t nowMillis, uint32_t randomVal)
{
    char sendData[TEST_STRING_SIZE];
    snprintf(sendData, TEST_STRING_SIZE, "%d,\t%lu * %d =\t%lu",
             counter, randomVal, counter, randomVal * counter);

    ROBO_WCOM::SendPacket(nowMillis, reinterpret_cast<uint8_t*>(sendData), strlen(sendData) + 1);
    Serial.print(sendData);
}

void testReceive(uint32_t nowMillis)
{
    char receivedString[128];
    uint8_t size;
    int32_t timestamp;
    uint8_t address[6];
    uint8_t data[ROBO_WCOM::CARRIED_DATA_MAX_SIZE];

    int rcv_status = ROBO_WCOM::ReceivePacket(nowMillis, &timestamp, address, data, &size);

    switch (rcv_status)
    {
    case 0:
        Serial.print("\t || RECEIVE OK    ");
        break;
    case -1:
        Serial.print("\t || TIMEOUT ERROR ");
        break;
    case -2:
        Serial.print("\t || CRC ERROR     ");
        break;
    default:
        Serial.print("\t || UNKNOWN ERROR ");
        break;
    }

    snprintf(receivedString, sizeof(receivedString),
             "|| Received: time=%ld,\taddr=%02X:%02X:%02X:%02X:%02X:%02X, data=%s,\tsize=%d",
             timestamp,
             address[0], address[1], address[2],
             address[3], address[4], address[5],
             data, size);

    Serial.print(receivedString);
}
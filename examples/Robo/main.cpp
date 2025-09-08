/***************************************************************************************************/
/*=========================== ROBO ================================================================*/
/***************************************************************************************************/
#include <Arduino.h>
#include "ROBO_WCOM.h"

struct __attribute__((packed)) Command{
    int16_t vx_i; // LSB:100/256/256
    int16_t yx_i; // 
    int16_t vw_i; // 
};

//---------------------------------------------
// 通信相手のMACアドレス設定
//---------------------------------------------
uint8_t MACADDRESS_BOARD_CONTROLLER[6] = {  0x08, 0xB6, 0x1F, 0xEE, 0x42, 0xF4};
uint8_t MACADDRESS_BOARD_ROBO[6] = {        0xEC, 0xE3, 0x34, 0xD1, 0x36, 0xBC};

void setup()
{
    Serial.begin(115200);
    Serial.println("Board Boot");

#if BOARD_IS == BOARD_A
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

    // パケット受信
//    auto rcv_status = ROBO_WCOM::PopOldestPacket(nowMillis, &timestamp, address, data, &size);
    auto rcv_status = ROBO_WCOM::PeekLatestPacket(nowMillis, &timestamp, address, data, &size);

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

#include <Arduino.h>
#include "ROBO_WCOM.h"

#define TEST_STRING_SIZE 64
#define SEND_RECEVE_MODE  10
#define SEND_ONLY_MODE  100

uint8_t MACADDRESS_BOARD_A[6] = {   0x08,   0xB6,   0x1F,   0xEE,   0x42,   0xF4}; // 送信先のデバイスのアドレス(6バイト)
uint8_t MACADDRESS_BOARD_B[6] = {   0xEC,   0xE3,   0x34,   0xD1,   0x36,   0xBC}; // 送信先のデバイスのアドレス(6バイト)
int16_t testCounter = 0;
rPacketData_t receivedPacket;

int modeCounter;
void testSend(uint16_t testCounter, uint32_t nowMills, uint32_t randomVal);
void testReceive(uint32_t nowMills);

/***************************************************************************************************/
/*=========================== ROBO ================================================================*/
/***************************************************************************************************/
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Robo communication started.");
    Serial.println(ROBO_WCOM_Init(MACADDRESS_BOARD_B, MACADDRESS_BOARD_A, 1000));
}

/***************************************************************************************************/
/*=========================== ROBO ================================================================*/
/***************************************************************************************************/
void loop()
{
// put your main code here, to run repeatedly:
    uint32_t  randomVal;
    uint32_t nowMills = millis();
    int rcv_status;

    randomVal = random(0, 100);
    
    // if (modeCounter < SEND_RECEVE_MODE)
    // {
    //     // 送信
    //     testSend(uint16_t testCounter, uint32_t nowMills, uint32_t randomVal);
    //     delay(10);
    // }
    // else if (modeCounter < (SEND_RECEVE_MODE + SEND_ONLY_MODE))
    // {
        
    // }
    // else
    // {
        
    // }
    testSend(testCounter, nowMills, randomVal);
    delay(10);
    testReceive(nowMills);
    Serial.println();
    testCounter++;
    modeCounter++;
}

void testSend(uint16_t testCounter, uint32_t nowMills, uint32_t randomVal)
{
    char sendData[TEST_STRING_SIZE];
    snprintf(sendData, TEST_STRING_SIZE, "%d,\t%ld * %d =\t%ld", testCounter, randomVal, testCounter, randomVal * testCounter);
    ROBO_WCOM_SendPacket(nowMills, (uint8_t*)sendData, strlen(sendData)+1);
    Serial.print(sendData);
}

void testReceive(uint32_t nowMills)
{
    char receivedString[128];
    int rcv_status;
    rcv_status = ROBO_WCOM_ReceivePacket(nowMills, (int32_t*)&receivedPacket.timestamp, receivedPacket.address, receivedPacket.carriedData, &receivedPacket.carriedSize);
    switch(rcv_status)
    {
        case 0:
            Serial.print("\t || RECEIVE OK    ");
            break;
        case -1:
            Serial.print("\t ||  TIMEOUT ERROR ");
            break;
        case -2:
            Serial.print("\t ||  CRC     ERROR ");
            break;
        default:
            Serial.print("\t ||  UNKNOWN ERROR ");
            break;
    }
    snprintf(   receivedString, 128, "|| Received: time=%ld,\taddr=%02X:%02X:%02X:%02X:%02X:%02X, data=%s,\tsize=%d", receivedPacket.timestamp,
                receivedPacket.address[0], receivedPacket.address[1], receivedPacket.address[2],
                receivedPacket.address[3], receivedPacket.address[4], receivedPacket.address[5],
                receivedPacket.carriedData, receivedPacket.carriedSize);
    Serial.print(receivedString);
}

/***************************************************************************************************/
/*=========================== Controller ==========================================================*/
/***************************************************************************************************/
#include <Arduino.h>
#include "ROBO_WCOM.h"
#include "controller_packet.h"
#include "robo_packet.h"


//---------------------------------------------
// 通信相手のMACアドレス設定
//---------------------------------------------
uint8_t MACADDRESS_BOARD_CONTROLLER[6] = {  0x08, 0xB6, 0x1F, 0xEE, 0x42, 0xF4};
uint8_t MACADDRESS_BOARD_ROBO[6] = {        0xEC, 0xE3, 0x34, 0xD1, 0x36, 0xBC};

// //
// float battery_voltage;
// float power_current;
// float wh;

// // 
#define MOTOR_CH_FL 0
#define MOTOR_CH_FR 1
#define MOTOR_CH_RL 2
#define MOTOR_CH_RR 3
// float motor_power[3];

// // 
// uint8_t wp_flg;
// uint16_t sw_flg;

// bool rcvStatus;

RoboCommand sendCommand;
RoboStatus rcvStatus;
uint32_t rcvTimeStamp;
uint8_t rcvAddress[6];
uint8_t rcvSize;

void setup()
{
    Serial.begin(115200);
    Serial.println("Board Boot");
    Serial.println("==== THIS IS CONTROLLER ====");

    auto initStatus = ROBO_WCOM::Init(MACADDRESS_BOARD_CONTROLLER, MACADDRESS_BOARD_ROBO, millis(), 1000);
    Serial.print("Controller started. : Status=");
    Serial.println(ROBO_WCOM::ToString(initStatus));
}

float vx;
bool vx_mode;
float vw;
bool vw_mode;

void loop()
{
    char reportedString[128];
    uint8_t wp = random(0, 0xFF);
    if (vx_mode)
    {
        vx += 0.05;
    }
    else
    {
        vx -= 0.05;
    }
    
    if (vw_mode)
    {
        vw += 0.01;
    }
    else
    {
        vw -= 0.01;
    }
    
    if (vx < 100.0)
    {
        vx_mode = true;
    }
    if (100.0 < vx)
    {
        vx_mode = false;
    }
    
    if (vw < 100.0)
    {
        vw_mode = true;
    }
    else
    {
        vw_mode = false;
    }
    
    sendCommand.velocity_x = vx;
    sendCommand.velocity_y = 0;
    sendCommand.omega = vw;
    sendCommand.WEAPON_FLAGS.FLAGS = wp;
    ROBO_WCOM::SendPacket(millis(), reinterpret_cast<uint8_t*>(&sendCommand), sizeof(RoboCommand));
    while(ROBO_WCOM::PopOldestPacket(millis(), &rcvTimeStamp, rcvAddress, reinterpret_cast<uint8_t*>(&rcvStatus), &rcvSize) != ROBO_WCOM::Status::BufferEmpty)
    {
        snprintf(reportedString, 128, "Time,%ld,POWER,%f,%f,%f,MOTORS,%f,%f,%f,%f,FLAGS,%0x,%0x", rcvTimeStamp, rcvStatus.Power.voltage, rcvStatus.Power.current, rcvStatus.Power.wh, rcvStatus.motors[MOTOR_CH_FL], rcvStatus.motors[MOTOR_CH_FR], rcvStatus.motors[MOTOR_CH_RL], rcvStatus.motors[MOTOR_CH_RR], rcvStatus.WEAPON_FLAGS.FLAGS, rcvStatus.MANSWICH.SWITCHES);
        Serial.println(reportedString);
    }
    delay(50);
}
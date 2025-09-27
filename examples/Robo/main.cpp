/***************************************************************************************************/
/*=========================== ROBO ================================================================*/
/***************************************************************************************************/
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "ROBO_WCOM.h"
#include <robo_packet.h>
#include <controller_packet.h>

#define CH_IRQ_TIMER    1
#define CONFORG_TIMER_DIV       80
#define CONFORG_TIMER_PRD_10MS  10000
#define CONFORG_TASK0_PRD_US    20000
#define CONFORG_TASK1_PRD_US    20000

//---------------------------------------------
//  タスクハンドラ
//---------------------------------------------
TaskHandle_t taskHandler[2];
hw_timer_t* hwtimer = NULL;

//---------------------------------------------
// 通信相手のMACアドレス設定
//---------------------------------------------
uint8_t MACADDRESS_BOARD_CONTROLLER[6] = {  0x08, 0xB6, 0x1F, 0xEE, 0x42, 0xF4};
uint8_t MACADDRESS_BOARD_ROBO[6] = {        0xEC, 0xE3, 0x34, 0xD1, 0x36, 0xBC};

// パワー関係の変数
float battery_voltage;
float power_current;
float wh;

// モータのCH
#define MOTOR_CH_FL 0
#define MOTOR_CH_FR 1
#define MOTOR_CH_RL 2
#define MOTOR_CH_RR 3
float motor_power[3];
uint8_t wp_flg;
uint16_t sw_flg;

/**送受信するデータ**/
RoboCommand rcvCommand;
RoboStatus sendStatus;

void MainTaskCore0(void *pvParameters);
void MainTaskCore1(void *pvParameters);
void TimerInterrupt(void);

void setup()
{
    Serial.begin(115200);
    Serial.println("Board Boot");
    Serial.println("==== THIS IS ROBO ====");

    auto initStatus = ROBO_WCOM::Init(MACADDRESS_BOARD_ROBO, MACADDRESS_BOARD_CONTROLLER, millis(), 1000);
    Serial.print("Communication started. : Status=");
    Serial.println(ROBO_WCOM::ToString(initStatus));

    hwtimer = timerBegin(CH_IRQ_TIMER, CONFORG_TIMER_DIV, true);
    timerAttachInterrupt(hwtimer, &TimerInterrupt, true);
    timerAlarmWrite(hwtimer, CONFORG_TIMER_PRD_10MS, true);
    timerAlarmEnable(hwtimer);
    xTaskCreateUniversal(   MainTaskCore0,  "MainTaskCore0",
                    /*      スタックサイズ  タスクに渡すパラメータ(?) */
                            8192,           NULL,
                    /*      優先度          タスクハンドラ      配置するコア */
                            1,              &taskHandler[0],    0);

                    /*      関数名          関数名(文字列) */
    xTaskCreateUniversal(   MainTaskCore1,  "MainTaskCore1",
                    /*      スタックサイズ  タスクに渡すパラメータ(?) */
                            8192,           NULL,
                    /*      優先度          タスクハンドラ      配置するコア */
                            2,              &taskHandler[1],    1);
}


/** Report and Logging **/
void loop()
{
    uint32_t nowMillis = millis();
    char loggingString[128];
    // コントローラ側に送り返すデータをローカルでも表示しておく
    snprintf(loggingString, 128,    "RoboTime,%ld,POWER,%f,%f,%f,MOTORS,%f,%f,%f,%f,FLAGS,0x%02X,0x%04X",
                                    nowMillis,
                                    battery_voltage, power_current, wh,
                                    motor_power[MOTOR_CH_FL], motor_power[MOTOR_CH_FR], motor_power[MOTOR_CH_RL], motor_power[MOTOR_CH_RR], 
                                    wp_flg, sw_flg);
    Serial.println(loggingString);
    
    // コントローラ側へ送信するデータをまとめる
    sendStatus.Power.voltage = battery_voltage;
    sendStatus.Power.current = power_current;
    sendStatus.Power.wh = wh;
    sendStatus.motors[MOTOR_CH_FL] = motor_power[MOTOR_CH_FL];
    sendStatus.motors[MOTOR_CH_FR] = motor_power[MOTOR_CH_FR];
    sendStatus.motors[MOTOR_CH_RL] = motor_power[MOTOR_CH_RL];
    sendStatus.motors[MOTOR_CH_RR] = motor_power[MOTOR_CH_RR];
    sendStatus.WEAPON_FLAGS.FLAGS = wp_flg;
    sendStatus.MANSWICH.SWITCHES = sw_flg;
    ROBO_WCOM::SendPacket(millis(), reinterpret_cast<uint8_t*>(&sendStatus), sizeof(sendStatus));
    delay(50);
}

/** Measurement **/
/**計測関係は同時性が大事なのでハードウェア割り込みに入れておく**/
void TimerInterrupt(void)
{
    // 仮想的に電流を計測したことにする(モータが動けば電流を消費するイメージ)
    float current = (   motor_power[MOTOR_CH_FL] * motor_power[MOTOR_CH_FL] +
                        motor_power[MOTOR_CH_FR] * motor_power[MOTOR_CH_FR] +
                        motor_power[MOTOR_CH_RL] * motor_power[MOTOR_CH_RL] +
                        motor_power[MOTOR_CH_RR] * motor_power[MOTOR_CH_RR] ) * 0.1;
    // 電流に応じて12Vの電源電圧が減る想定
    float voltage = 12 - 0.1 * current;

    battery_voltage = voltage;
    power_current = current;
    
    // 積算電力を計算しておく
    wh += current * voltage * 0.01 / 3600;
}

// メインコントロール
// 受信したデータに応じてロボットを制御する
void MainTaskCore0(void *pvParameters)
{
    uint32_t rcvTimeStamp;
    uint8_t controllerAddress[6];
    uint8_t rcvSize;
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20);
    while(1)
    {
        auto rcvStatus = ROBO_WCOM::PeekLatestPacket(millis(), &rcvTimeStamp, controllerAddress, reinterpret_cast<uint8_t*>(&rcvCommand), &rcvSize);
        if (rcvStatus == ROBO_WCOM::Status::Ok)
        {
            motor_power[MOTOR_CH_FL] = rcvCommand.velocity_x - rcvCommand.omega;
            motor_power[MOTOR_CH_FR] = rcvCommand.velocity_x + rcvCommand.omega;
            motor_power[MOTOR_CH_RL] = motor_power[MOTOR_CH_FL];
            motor_power[MOTOR_CH_RR] = motor_power[MOTOR_CH_FR];
            wp_flg = rcvCommand.WEAPON_FLAGS.FLAGS;
        }
        // 正常な受信ができていない場合モータを止めておく
        else
        {
            motor_power[MOTOR_CH_FL] = 0;
            motor_power[MOTOR_CH_FR] = 0;
            motor_power[MOTOR_CH_RL] = 0;
            motor_power[MOTOR_CH_RR] = 0;
            wp_flg = 0x00;
        }
        vTaskDelayUntil(&lastWake, period);
    }

}

// UIの管理
void MainTaskCore1(void *pvParameters)
{
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(50);
    while(1)
    {
        // ランダムに押される状況を想定
        sw_flg = random(0, 0xFFFF);
        vTaskDelayUntil(&lastWake, period);
    }
}


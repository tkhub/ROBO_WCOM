/***************************************************************************************************/
/*=========================== Controller ==========================================================*/
/***************************************************************************************************/
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ROBO_WCOM.h"
#include "controller_packet.h"
#include "robo_packet.h"


//---------------------------------------------
// 通信相手のMACアドレス設定
//---------------------------------------------
uint8_t MACADDRESS_BOARD_CONTROLLER[6] = {  0x08, 0xB6, 0x1F, 0xEE, 0x42, 0xF4};
uint8_t MACADDRESS_BOARD_ROBO[6] = {        0xEC, 0xE3, 0x34, 0xD1, 0x36, 0xBC};


/** モータ配列のインデックス値 **/
#define MOTOR_CH_FL 0
#define MOTOR_CH_FR 1
#define MOTOR_CH_RL 2
#define MOTOR_CH_RR 3

void statusViewer(void* pvParameters);  // ROBO側のステータスを取得して表示
TaskHandle_t thp[1];                    // タスクハンドラ

RoboCommand sendCommand;                // 送信するコマンド
RoboStatus rcvStatus;                   // 受信したステータス
uint32_t rcvTimeStamp;                  // 受信したロボット側の時刻
uint8_t rcvAddress[6];                  // 受信したロボット側のアドレス
uint8_t rcvSize;                        // 受信したデータのサイズ

void setup()
{
    Serial.begin(115200);
    Serial.println("Board Boot");
    Serial.println("==== THIS IS CONTROLLER ====");
//    コア指定なし（どちらのコアでもスケジュール可能）
    xTaskCreateUniversal(
        statusViewer,       // タスク関数
        "StatusViewer",
        4096,                // スタックサイズ
        nullptr,             // パラメータ
        1,                   // 優先度
        nullptr,             // ハンドル不要
        tskNO_AFFINITY       // コア指定なし
    );

    auto initStatus = ROBO_WCOM::Init(MACADDRESS_BOARD_CONTROLLER, MACADDRESS_BOARD_ROBO, millis(), 1000);
    Serial.print("Controller started. : Status=");
    Serial.println(ROBO_WCOM::ToString(initStatus));
}

/**
 * @brief ステータス表示用のタスク
 * 
 * @param pvParameters
 */
void statusViewer(void* pvParameters)
{
    char reportedString[128];
    TickType_t lastWake = xTaskGetTickCount();
    // 実行周期=50ms
    const TickType_t period = pdMS_TO_TICKS(50);
    ROBO_WCOM::Status readBufferStatus;

    for (;;)
    {
        // ロボット側からの受信データをバッファから取り出す
        for (;;)
        {
            readBufferStatus = ROBO_WCOM::PopOldestPacket(millis(), &rcvTimeStamp, rcvAddress, reinterpret_cast<uint8_t*>(&rcvStatus), &rcvSize);
            // バッファからデータを取り出せなくなったら受信を中止
            if (readBufferStatus == ROBO_WCOM::Status::BufferEmpty)
            {
                break;
            }
            // タイムアウト状態の場合それを通知し受信を中止
            else if (readBufferStatus == ROBO_WCOM::Status::Timeout)
            {
                Serial.println("CONNECTION REFUSE");
                break;
            }
            // CRCエラーの場合エラーを通知
            else if (readBufferStatus == ROBO_WCOM::Status::CrcError)
            {
                Serial.println("!!CRC ERROR!!");
            }
            // 正常に受信できているならデータを
            else
            {
                snprintf(reportedString, 128, "Time,%ld,POWER,%f,%f,%f,MOTORS,%f,%f,%f,%f,FLAGS,%0x,%0x",
                                                rcvTimeStamp,
                                                rcvStatus.Power.voltage, rcvStatus.Power.current, rcvStatus.Power.wh,
                                                rcvStatus.motors[MOTOR_CH_FL], rcvStatus.motors[MOTOR_CH_FR], rcvStatus.motors[MOTOR_CH_RL], rcvStatus.motors[MOTOR_CH_RR],
                                                rcvStatus.WEAPON_FLAGS.FLAGS, rcvStatus.MANSWICH.SWITCHES);
                Serial.println(reportedString);
            }
        }
        vTaskDelayUntil(&lastWake, period);
    }
}


float vx;
bool vx_mode;
float vw;
bool vw_mode;
uint8_t wp;

void loop(void)
{
    char cmdString[32];
    uint32_t nowMillis;
    // 武器フラグは乱数にしておく
    wp = random(0, 0xFF);
    // vx, vyを -100~100の間で増減させる
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
    
    // ロボット側へ送信するデータをまとめて送信
    sendCommand.velocity.x = vx;
    sendCommand.velocity.y = 0;
    sendCommand.velocity.omega = vw;
    sendCommand.WEAPON_FLAGS.FLAGS = wp;
    nowMillis = millis();
    ROBO_WCOM::SendPacket(nowMillis, reinterpret_cast<uint8_t*>(&sendCommand), sizeof(RoboCommand));
    
    // デバッグ用に送信した時刻だけ表示する
    snprintf(cmdString, 32, "<< SEND CMD : %ld >>", nowMillis);
    Serial.println(cmdString);
    delay(20);
}
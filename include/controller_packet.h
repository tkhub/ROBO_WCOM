#ifndef __CONTROLLER_PACKET_H__
#define __CONTROLLER_PACKET_H__
#include <Arduino.h>

/**
 * @brief ロボットへの指令を表す通信パケット構造体
 * 
 * この構造体は、移動指令・武器操作・LED制御・HP情報など、
 * ロボットに送信する各種コマンドを一括で表現します。
 * 通信時のバイト整列を保証するため、packed属性が付与されています。
 */
typedef struct __attribute__((packed)) RoboCommand_st
{
    /**
     * @brief 移動速度指令
     * 
     * ロボットの移動方向および回転速度を相対値で指定。
     * 値の範囲は -100 ～ +100 を想定（正規化済み）。
     */
    struct __attribute__((packed))
    {
        float x;       ///< x軸方向の速度（前後移動） [-100 ~ +100]
        float y;       ///< y軸方向の速度（左右移動） [-100 ~ +100]
        float omega;   ///< 角速度（回転） [-100 ~ +100]
    } velocity;

    /**
     * @brief 武器操作フラグ
     * 
     * 武器のON/OFF状態をビットフラグで管理。全体を一括で操作するFLAGSと、
     * 左右個別の武器フラグにアクセスできるFLAG構造体を持つ。
     */
    union
    {
        uint8_t FLAGS;  ///< 武器フラグ全体（8bit）を一括操作

        struct
        {
            // 左側武器フラグ（WPL: Weapon Port Left）
            uint8_t WPL03:1;    ///< 左武器4の操作フラグ
            uint8_t WPL02:1;    ///< 左武器3の操作フラグ
            uint8_t WPL01:1;    ///< 左武器2の操作フラグ
            uint8_t WPL00:1;    ///< 左武器1の操作フラグ

            // 右側武器フラグ（WPR: Weapon Port Right）
            uint8_t WPR03:1;    ///< 右武器4の操作フラグ
            uint8_t WPR02:1;    ///< 右武器3の操作フラグ
            uint8_t WPR01:1;    ///< 右武器2の操作フラグ
            uint8_t WPR00:1;    ///< 右武器1の操作フラグ
        } FLAG;
    } WEAPON_FLAGS;

    /**
     * @brief LED制御情報
     * 
     * フルカラーLEDの色指定。RGB_DATASで一括指定するか、
     * RGB構造体で個別の色成分にアクセス可能。
     */
    union
    {
        uint32_t RGB_DATAS;  ///< RGB値を32bitで一括指定（例：0x00RRGGBB）

        struct
        {
            uint8_t RED;     ///< 赤成分（0～255）
            uint8_t GREEN;   ///< 緑成分（0～255）
            uint8_t BLUE;    ///< 青成分（0～255）
        } RGB;
    } RGBLED;

    /**
     * @brief ロボットのHP情報
     * 
     * ゲーム的な要素や状態管理に使用されるHP（Health Point）。
     * 通常は0～65535の範囲で管理。
     */
    uint16_t hp;  ///< ロボットのHP（体力・耐久値）
} RoboCommand_t;

#endif /* __CONTROLLER_PACKET_H__ */

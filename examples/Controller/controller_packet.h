#ifndef __CONTROLLER_PACKET_H__
#define __CONTROLLER_PACKET_H__

#include <Arduino.h>

struct __attribute__((packed)) RoboCommand{

    struct __attribute__((packed))
    {
        float x;           ///<  x軸方向の速度（相対値） -100 ~ +100
        float y;           ///<  y軸方向の速度（相対値） -100 ~ +100
        float omega;                ///<  角速度（相対値） -100 ~ +100
    }velocity;
    union
    {
        uint8_t FLAGS;          ///< 武器フラグを一括操作
        struct
        {
            // 左側武器フラグ
            uint8_t WPL03:1;    ///< 左武器4
            uint8_t WPL02:1;    ///< 左武器3
            uint8_t WPL01:1;    ///< 左武器2
            uint8_t WPL00:1;    ///< 左武器1

            // 右側武器フラグ
            uint8_t WPR03:1;    ///< 右武器4
            uint8_t WPR02:1;    ///< 右武器3
            uint8_t WPR01:1;    ///< 右武器2
            uint8_t WPR00:1;    ///< 右武器1
        }FLAG;
    }WEAPON_FLAGS;
    union
    {
        uint32_t RGB_DATAS;     ///< フルカラーLEDの一括変更
        struct
        {
            uint8_t RED;
            uint8_t GREEN;
            uint8_t BLUE;
        }RGB;
    }RGBLED;
    uint16_t hp;                ///< HP
};

#endif /* __CONTROLLER_PACKET_H__ */

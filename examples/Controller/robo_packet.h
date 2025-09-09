#ifndef __ROBO_PACKET_H__
#define __ROBO_PACKET_H__

#define MOTOR_NUM 8

struct __attribute__((packed)) RoboStatus
{
    struct __attribute__((packed))
    {
        float voltage;          ///< 電源電圧
        float current;          ///< 電源電流
        float wh;               ///< 電力量
    }Power;
    float motors[MOTOR_NUM];    ///< モータの稼働状況
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
        uint16_t SWITCHES;
        struct
        {
            uint8_t S4      : 1;
            uint8_t S3      : 1;
            uint8_t S2      : 1;
            uint8_t S1      : 1;
            uint8_t SQUARE  : 1;
            uint8_t CIRCLE  : 1;
            uint8_t CROSS   : 1;
            uint8_t TRIANGLE: 1;
            uint8_t LEFT    : 1;
            uint8_t RIGHT   : 1;
            uint8_t DOWN    : 1;
            uint8_t UP      : 1;
        }BUTTON;
    }MANSWICH;
};

#endif /* __ROBO_PACKET_H__ */

#ifndef __ROBO_PACKET_H__
#define __ROBO_PACKET_H__

#define MOTOR_NUM 8

/**
 * @brief ロボットの状態を表す通信パケット構造体
 * 
 * この構造体は、電源情報、モータの稼働状況、武器フラグ、操作スイッチなど、
 * ロボットの現在の状態を網羅的に表現するために使用されます。
 * 通信時のバイト整列を保証するため、packed属性が付与されています。
 */
typedef struct __attribute__((packed)) RoboStatus_st
{
    /**
     * @brief 電源情報
     * 
     * 電圧・電流・電力量を含み、ロボットの電源状態を監視するために使用されます。
     */
    struct __attribute__((packed))
    {
        float voltage;          ///< 電源電圧 [V]
        float current;          ///< 電源電流 [A]
        float wh;               ///< 消費電力量 [Wh]
    } Power;

    /**
     * @brief モータの稼働状況
     * 
     * 各モータの状態を表す配列。インデックスはMOTOR_NUMで定義されたモータ数に対応。
     */
    float motors[MOTOR_NUM];    ///< モータの稼働状況（例：速度、出力など）

    /**
     * @brief 武器フラグ情報
     * 
     * 武器のON/OFF状態をビットフラグで管理。全体を一括で操作するFLAGSと、
     * 左右個別の武器フラグにアクセスできるFLAG構造体を持つ。
     */
    union
    {
        uint8_t FLAGS;          ///< 武器フラグ全体（8bit）を一括操作

        struct
        {
            // 左側武器フラグ（WPL: Weapon Port Left）
            uint8_t WPL03:1;    ///< 左武器4の状態
            uint8_t WPL02:1;    ///< 左武器3の状態
            uint8_t WPL01:1;    ///< 左武器2の状態
            uint8_t WPL00:1;    ///< 左武器1の状態

            // 右側武器フラグ（WPR: Weapon Port Right）
            uint8_t WPR03:1;    ///< 右武器4の状態
            uint8_t WPR02:1;    ///< 右武器3の状態
            uint8_t WPR01:1;    ///< 右武器2の状態
            uint8_t WPR00:1;    ///< 右武器1の状態
        } FLAG;
    } WEAPON_FLAGS;

    /**
     * @brief 操作スイッチ情報
     * 
     * コントローラの各ボタンの状態を管理。SWITCHESで全体を一括取得可能。
     * BUTTON構造体で個別のボタン状態にアクセス可能。
     */
    union
    {
        uint16_t SWITCHES;      ///< スイッチ状態全体（16bit）

        struct
        {
            // トリガー系スイッチ
            uint8_t S4      : 1; ///< スイッチ4
            uint8_t S3      : 1; ///< スイッチ3
            uint8_t S2      : 1; ///< スイッチ2
            uint8_t S1      : 1; ///< スイッチ1

            // ボタン系（PlayStation風レイアウト）
            uint8_t SQUARE  : 1; ///< 四角ボタン
            uint8_t CIRCLE  : 1; ///< 丸ボタン
            uint8_t CROSS   : 1; ///< ×ボタン
            uint8_t TRIANGLE: 1; ///< △ボタン

            // 十字キー系
            uint8_t LEFT    : 1; ///< 左
            uint8_t RIGHT   : 1; ///< 右
            uint8_t DOWN    : 1; ///< 下
            uint8_t UP      : 1; ///< 上
        } BUTTON;
    } MANSWICH;
} RoboStatus_t;


#endif /* __ROBO_PACKET_H__ */

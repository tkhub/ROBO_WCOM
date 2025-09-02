#ifndef __ROBO_WCOM_H__
#define __ROBO_WCOM_H__

#include <Arduino.h>

#define CARRIED_DATA_MAX_SIZE 200

typedef struct  __attribute__((packed)) {
    uint32_t timestamp;        // 受信時刻（millis()）
    uint8_t address[6];     // 送信元のデバイスのアドレス(6バイト)
    uint8_t  carriedSize;      // 搬送データサイズ
    uint8_t  carriedData[CARRIED_DATA_MAX_SIZE]; // 搬送データ本体
} rPacketData_t;


#ifdef	__GLOBAL_DEFINE__
    /* extern uint8_t hogehoge */
    
#endif	/* __GLOBAL_DEFINE__ */
#ifdef	__GLOBAL_DEFINE__
    #define	__GLOBAL
#else	/* NOT DEFINE __GLOBAL_DEFINE__ */
    #define	__GLOBAL extern
#endif	/* __GLOBAL_DEFINE__ */
    /* __GLOBAL void test(void);*/
    /* [after __GLOBAL_DEFINE__ ] */
    /* void test(void); */
    /* [after __GLOBAL_DEFINE__ is NOT defined.] */
    /* extern void test(void); */
    /* __GLOBAL void test(void);*/
    
    __GLOBAL int ROBO_WCOM_Init(uint8_t sourceAddress[6], uint8_t distAddress[6], uint32_t timeoutMS);
    __GLOBAL int ROBO_WCOM_SendPacket(uint32_t timestamp, const uint8_t* data, uint8_t size);
    __GLOBAL int ROBO_WCOM_ReceivePacket(uint32_t nowMills, int32_t* timestamp, uint8_t *address, uint8_t* data, uint8_t* size);
    __GLOBAL bool ROBO_WCOM_isAvailableReceivePacket(void);
    
#ifdef	__GLOBAL_DEFINE__
    #undef	__GLOBAL_DEFINE__
#endif	/* __GLOBAL_DEFINE__ */
#undef	__GLOBAL



#endif /* __ROBO_WCOM_H__ */
/*
 * State.h
 *
 *  Created on: 2020/10/20
 *      Author: fuminori.hakoishi
 */

#ifndef INC_STATE_H_
#define INC_STATE_H_

#include "main.h"

typedef enum {
    SYS_STATE_TEMP,           // 2つの温度センサの値を表示する画面
    SYS_STATE_DISTANCE,       // 距離センサの値を表示する画面
    SYS_STATE_DEBUG_POINTER,  // ポインタのデバッグ用
    SYS_STATE_DEBUG_RECIEVE,  // UARTの受信デバッグ用

    SYS_STATE_BLE = 4,            // BLEテスト
    SYS_STATE_BLE_CENTRAL = 5,      // BLE セントラル側
    SYS_STATE_DEBUG,   // UARTにデバッグ用の出力をする
    SYS_STATE_LENGTH,  // システムの状態の数
}
LCD_State_Type;

void State_Init(uint8_t i_state);
void State_ChangeStateRoll(void);
void State_RunRealtimeProcess(void);
void State_RunProcess(void);

void State_SetUpdateFlg(void);

uint8_t State_GetState(void);

#endif /* INC_STATE_H_ */

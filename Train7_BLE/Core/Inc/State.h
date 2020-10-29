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
    SYS_STATE_DEBUG_RECIEVE = 0,  // UARTの受信デバッグ用
    SYS_STATE_BLE,                // BLEテスト
    SYS_STATE_BLE_CENTRAL,        // BLE セントラル側
    SYS_STATE_LENGTH,             // システムの状態の数
} LCD_State_Type;

void State_Init(uint8_t i_state);
void State_ChangeStateRoll(void);
void State_RunRealtimeProcess(void);
void State_RunProcess(void);

void State_SetUpdateFlg(void);

uint8_t State_GetState(void);

#endif /* INC_STATE_H_ */

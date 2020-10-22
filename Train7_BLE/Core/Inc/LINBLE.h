/*
 * LINBLE.h
 *
 *  Created on: Oct 19, 2020
 *      Author: fuminori.hakoishi
 */

#ifndef INC_LINBLE_H_
#define INC_LINBLE_H_

#include "main.h"
#include "stdbool.h"

void LINBLE_Init(UART_HandleTypeDef *huart);
void LINBLE_SetReceiveData(void);
int8_t LINBLE_StartConnection(void);
int8_t LINBLE_ShowVersion(void);
int8_t LINBLE_ShowDeviceName(void);
uint8_t LINBLE_GetState(void);
uint8_t LINBLE_SetState(uint8_t i_state);

// リザルトメッセージ待機フラグ
bool LINBLE_GetReceiveResultMesgWaitFlg(void);
void LINBLE_ClrReceiveWaitFlg(void);

// エンドラインフラグ
bool LINBLE_GetEndLineFlg(void);
bool LINBLE_CleEndLineFlg(void);

void LINBLE_BufferCountClear(void);

void LINBLE_EnterHandler(uint8_t i_sysState);

uint8_t LINBLE_GetReceiveCharLast(void);
int8_t LINBLE_SendCommandToLINBLE(uint8_t *i_cmd, uint8_t i_cmdSize);

// バッファに入っているデータを取得する
uint8_t LINBLE_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize);

typedef enum {
    LINBLE_STATE_COMMAND,
    LINBLE_STATE_ADVERTISE,
    LINBLE_STATE_ONLINE,
} LINBLE_State_Type;

#endif /* INC_LINBLE_H_ */

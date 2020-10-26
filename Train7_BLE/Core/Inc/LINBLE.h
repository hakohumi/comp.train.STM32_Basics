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
int8_t LINBLE_SendCmdStartConnection(void);
int8_t LINBLE_SendCmdShowVersion(void);
int8_t LINBLE_SendCmdShowDeviceName(void);
int8_t LINBLE_SendCmdCheckStatus(void);
int8_t LINBLE_SendCmdScanDevice(void);
int8_t LINBLE_SendCmdConnectPeripheral(void);
uint8_t LINBLE_GetState(void);
uint8_t LINBLE_SetState(uint8_t i_state);

// リザルトメッセージ待機フラグ
bool LINBLE_GetReceiveResultMesgWaitFlg(void);
void LINBLE_ClrReceiveResultMesgWaitFlg(void);

// エンドラインフラグ
bool LINBLE_GetEndLineFlg(void);
bool LINBLE_CleEndLineFlg(void);

bool LINBLE_GetEscapeStateFlg(void);
void LINBLE_SetEscapeStateFlg(void);
void LINBLE_ClrEscapeStateFlg(void);

void LINBLE_BufferCountClear(void);

void LINBLE_EnterHandler(uint8_t i_sysState);

uint8_t LINBLE_GetReceiveCharLast(void);
int8_t LINBLE_SendCmdStrToLINBLE(uint8_t *i_cmd, uint8_t i_cmdSize);

// バッファに入っているデータを取得する
uint8_t LINBLE_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize);

// 現在のバッファのインデックス位置を取得
uint8_t LINBLE_GetReceiveCountLast(void);

// コマンド実行フラグの構造体のポインタを返す
bool LINBLE_GetCmdFlg(uint8_t i_cmd);

int8_t LINBLE_ReceiveDataBTI(uint8_t *i_strBuf, uint8_t i_bufSize);

// ペリフェラルへの接続要求の受信待機
// btcフラグが立っている時の受信処理
int8_t LINBLE_ReceiveDataBTC(uint8_t *i_strBuf, uint8_t i_bufSize);

int8_t PrintLINBLE(uint8_t *i_str, uint8_t i_size);

typedef enum {
    LINBLE_STATE_COMMAND,
    LINBLE_STATE_ADVERTISE,
    LINBLE_STATE_ONLINE,
} LINBLE_State_Type;

typedef enum {
    LINBLE_FLG_CMD_BTI,
    LINBLE_FLG_CMD_BTC,
} LINBLE_FLG_CMD_E;

#endif /* INC_LINBLE_H_ */

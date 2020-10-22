/*
 * LINBLE.h
 *
 *  Created on: Oct 19, 2020
 *      Author: fuminori.hakoishi
 */

#ifndef INC_LINBLE_H_
#define INC_LINBLE_H_

#include "main.h"

void LINBLE_Init(UART_HandleTypeDef *huart);
void LINBLE_SetReceiveData(void);
int8_t LINBLE_StartConnection(void);
int8_t LINBLE_ShowVersion(void);
int8_t LINBLE_ShowDeviceName(void);
void LINBLE_Wait(void);
void LINBLE_EnterHandler(uint8_t i_sysState);

uint8_t LINBLE_GetReceiveCharLast(void);
int8_t LINBLE_SendCommandToLINBLE(uint8_t *i_cmd, uint8_t i_cmdSize);

// バッファに入っているデータを取得する
uint8_t LINBLE_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize);

#endif /* INC_LINBLE_H_ */

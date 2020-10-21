/*
 * UART.h
 *
 *  Created on: 2020/09/29
 *      Author: fuminori.hakoishi
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include <stdbool.h>

#include "main.h"

void UART_Console_Init(UART_HandleTypeDef *huart);
void UART_SetReceiveData(void);

void UART_ReceiveCountUp(void);

int8_t PrintUART(uint8_t *i_str);
int8_t PrintUARTn(uint8_t *i_str, uint8_t i_size);
int8_t PrintUARTInt(uint32_t i_var);

// デバッグ用
// 文字列と数字を表示
bool dprintUART(uint8_t *i_str, uint32_t i_var);
bool printUARTHex(uint8_t *i_str, uint32_t i_var, uint8_t i_len);

uint8_t UART_ReceiveInput(uint8_t i_state);

uint8_t UART_GetReceiveLineFlg(void);
void UART_ClrReceiveLineFlg(void);

void PrintERROR(uint8_t i_errorCode);

uint8_t *UART_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize);

typedef enum {
    ERROR_UART_PRINTUART_ENDOFLINE,
    ERROR_INPUT_OVERLENGTH,
    ERROR_INPUT_SHORTLENGTH,
    ERROR_INPUT_CONTROL_CODE,
    ERROR_INPUT_ALPHANUMERIC,
    ERROR_INPUT_HEX,
    ERROR_LINBLE_SENDFAILURE,        // 送信失敗
    ERROR_LINBLE_NOTFIND_ENDOFLINE,  // 終端文字が見当たらない
    ERROR_LINBLE_NOTFIND_CR,         // CRが見つからない
    ERROR_LINBLE_RECIEVEFAILURE,     // 受信準備失敗
    ERROR_MYSTRINGFUNC_NOTFIND_ENDOFLINE,
} ERROR_CODE_Type;

typedef enum {
    UART_STATE_NONPUSHED,
    UART_STATE_PUSHED_ENTER,
    UART_STATE_LENGTH
} UART_STATE_Type;

#endif /* INC_UART_H_ */

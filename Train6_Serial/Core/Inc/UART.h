/*
 * UART.h
 *
 *  Created on: 2020/09/29
 *      Author: fuminori.hakoishi
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include "main.h"
#include <stdbool.h>

void msgrx_init(UART_HandleTypeDef *huart);

// 未読データ数確認
int readDataNum(void);

uint8_t msgrx_circ_buf_get(void);
bool msgrx_circ_buf_is_empty(void);

void viewUART(void);

int8_t PrintUART(uint8_t *i_str);

// デバッグ用
// 文字列と数字を表示
bool dprintUART(uint8_t *i_str, uint32_t i_var);
bool printUARTHex(uint8_t *i_str, uint32_t i_var, uint8_t i_len);
void PrintERROR(uint8_t i_errorCode);

typedef enum {
	ERROR_UART_PRINTUART_ENDOFLINE,
	ERROR_INPUT_OVERLENGTH,
	ERROR_INPUT_SHORTLENGTH,
	ERROR_INPUT_CONTROL_CODE,
	ERROR_INPUT_ALPHANUMERIC,
	ERROR_INPUT_HEX,
} ERROR_CODE_Type;

#endif /* INC_UART_H_ */

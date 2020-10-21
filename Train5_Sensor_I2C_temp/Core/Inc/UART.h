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

void viewUART(void);

// デバッグ用
// 文字列と数字を表示
bool printUART(uint8_t *i_str, uint16_t i_var);

#endif /* INC_UART_H_ */

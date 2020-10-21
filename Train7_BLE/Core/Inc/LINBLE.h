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
int8_t LINBLE_StartConnection(void);
int8_t LINBLE_ShowVersion(void);
int8_t LINBLE_ShowDeviceName(void);

#endif /* INC_LINBLE_H_ */

/*
 * Temp_I2C.h
 *
 *  Created on: Oct 6, 2020
 *      Author: fuminori.hakoishi
 */

#ifndef INC_TEMP_I2C_H_
#define INC_TEMP_I2C_H_

#include "main.h"

// 温度センサの初期化
void TempI2C_Init(I2C_HandleTypeDef *i_hi2c);
int TempI2C_GetTemp(void);

#endif /* INC_TEMP_I2C_H_ */

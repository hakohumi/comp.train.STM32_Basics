/*
 * Temp_ADC.h
 *
 *  Created on: Oct 16, 2020
 *      Author: fuminori.hakoishi
 */

#ifndef INC_TEMP_ADC_H_
#define INC_TEMP_ADC_H_

#include "main.h"

void Temp_ADC_Init(uint16_t *i_DMABufArray);
int TEMP_ADC_GetTemp(void);

#endif /* INC_TEMP_ADC_H_ */

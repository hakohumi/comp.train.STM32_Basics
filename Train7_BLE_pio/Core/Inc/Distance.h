/*
 * Distance.h
 *
 *  Created on: Oct 16, 2020
 *      Author: fuminori.hakoishi
 */

#ifndef INC_DISTANCE_H_
#define INC_DISTANCE_H_

#include "main.h"

void Distance_Init(uint16_t *i_DMABufArray);
uint16_t Distance_ADC_GetDistance(void);

#endif /* INC_DISTANCE_H_ */

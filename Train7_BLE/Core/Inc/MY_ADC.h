/*
 * MY_ADC.h
 *
 *  Created on: 2020/10/29
 *      Author: fuminori.hakoishi
 */

#ifndef INC_MY_ADC_H_
#define INC_MY_ADC_H_

#include "main.h"

void MY_ADC_init(ADC_HandleTypeDef *hadc);

typedef enum {
    MY_ADC_DATA_IDX_EXTEMP   = 0,  // アナログ温度センサ
    MY_ADC_DATA_IDX_DISTANCE = 1,  // 距離センサ
    MY_ADC_DATA_IDX_INTEMP   = 2,  // マイコン内蔵温度センサ
    MY_ADC_DATA_IDX_VREFINT  = 3,  // マイコンのVREFの値
    MY_ADC_BUFFER_LENGTH,          // DMAのADC用バッファのサイズ
} MY_ADC_IDX_Type;

#endif /* INC_MY_ADC_H_ */

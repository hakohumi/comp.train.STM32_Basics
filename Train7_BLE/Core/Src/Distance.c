/*
 * Distance.c
 *
 *  Created on: Oct 16, 2020
 *      Author: fuminori.hakoishi
 */

#include "Distance.h"

uint16_t *g_ADCBuffer;

void Distance_Init(uint16_t *i_DMABufArray) {
	g_ADCBuffer = i_DMABufArray;
}

// 距離センサから読み取った値を計算して、距離(cm)に変換する
uint16_t Distance_ADC_GetDistance(void) {
	uint16_t l_rawValue;
	l_rawValue = g_ADCBuffer[ADC_DATA_IDX_DISTANCE];

	return (uint16_t) ((6787.0f * 4
			/ (float) (ADC_ConvertToVoltage(l_rawValue) - 3)) - 4);
//	2808.1 * powf((l_rawValue), -0.83));														// 実際の値から求めた（アナログ値）
//	2273.9 * powf(ADC_ConvertToVoltage(l_rawValue), -0.83));				// 実際の値から求めた（電圧）
//	26.757 * powf(ADC_ConvertToVoltage(l_rawValue) / 1000, -1.236));	// ネット
//	(-31.13 * log(ADC_ConvertToVoltage(l_rawValue) / 1000)) + 75.518);// データシート
//	(-31.13 * log(ADC_ConvertToVoltage(l_rawValue) / 1000)) + 75.518);// データシート
//	pow(ADC_ConvertToVoltage(l_rawValue), -1.20482) / 2273.9);
}

/*
 反比例の式
 y = k / x

 k(係数) / L(距離(cm)) = E[V](センサから出力される電圧）
 k = L * E = 300[mm] * 2[V] = 60[cm・V]

 analogReadの返す値をaとすると
 a = E[V] / 3.3[V] * 分解能（12bit, 4096) ,
 E[V] = a * 3.3[V] / 4096,
 L = k / E = k / ( a * 3.3 / 4096)
 = ( k * 4096 / 3.3 ) / a
 = ( k * 4096 / 3.3 ) / a
 = 27148 / a
 */

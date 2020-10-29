/*
 * Temp_ADC.c
 *
 *  Created on: Oct 16, 2020
 *      Author: fuminori.hakoishi
 */

#include "Temp_ADC.h"

#include "MY_ADC.h"

// 基準となる電圧
// ※テスターを使用して設定
#define ADC_VREF 3316

// ADCのアナログ値のオフセット
// ※テスターを使用して設定
//#define ADC_OFFSET_MIN	74
#define ADC_OFFSET_MIN 0
#define ADC_OFFSET_MAX 4095

uint16_t *g_ADCBuffer;

void Temp_ADC_Init(uint16_t *i_DMABufArray) {
    g_ADCBuffer = i_DMABufArray;
}

// ADC analog値 を 電圧値に変換する関数
uint16_t ADC_ConvertToVoltage(uint16_t i_analog) {
    float l_getVoltage;

    // 読み取ったアナログ値と分解能との比率を基準電圧に掛けて電圧を求める
    l_getVoltage = (((float)ADC_VREF * ((float)i_analog - (float)ADC_OFFSET_MIN)) / ((float)ADC_OFFSET_MAX - (float)ADC_OFFSET_MIN));

    //	printUART("ADC Voltage :", l_getVoltage);

    return (uint16_t)l_getVoltage;
}

// ADC温度センサ 温度取得
// ADC温度センサの電圧オフセット
#define ADCTEMP_VOLTAGE_OFFSET 600
// ADC温度センサの温度勾配
#define ADCTEMP_TEMP_SLOPE 10

// ADCで温度センサから温度を取得する
int TEMP_ADC_GetTemp(void) {
    int o_tmp = 0;
    uint16_t l_tmp;
    uint16_t l_getVoltage;

    // 12bit解像度で取得したADCの値
    // CPU使用
    // l_tmp = (uint16_t) HAL_ADC_GetValue(i_hadc);

    // DMA使用
    l_tmp = g_ADCBuffer[MY_ADC_DATA_IDX_EXTEMP];

    l_getVoltage = ADC_ConvertToVoltage(l_tmp);

    // 読み取った電圧から、計算用のために指定されているオフセット分の電圧を引く
    o_tmp = (int)(l_getVoltage - ADCTEMP_VOLTAGE_OFFSET);
    o_tmp /= ADCTEMP_TEMP_SLOPE;

    return o_tmp;
}

/*
 * ADC.c
 *
 *  Created on: 2020/10/29
 *      Author: fuminori.hakoishi
 */

#include "MY_ADC.h"

#include <string.h>
#include "Temp_ADC.h"
#include "Distance.h"
#include "UART.h"

static ADC_HandleTypeDef *this_hadc;

// ADC DMAバッファ
uint16_t G_ADCBuffer[MY_ADC_BUFFER_LENGTH];

void MY_ADC_init(ADC_HandleTypeDef *hadc) {
    // ADCのDMA用のバッファの初期化
    memset(G_ADCBuffer, 0, sizeof(G_ADCBuffer));

    this_hadc = hadc;

    // ADCのキャリブレーション
    // *ADCを無効にした状態で実行する必要がある
    HAL_ADCEx_Calibration_Start(this_hadc, ADC_SINGLE_ENDED);

    // アナログ温度センサの初期化
    Temp_ADC_Init((uint16_t *)&G_ADCBuffer);

    // アナログ距離センサの初期化
    Distance_Init((uint16_t *)&G_ADCBuffer);

    // ADC DMAスタート
    if (HAL_ADC_Start_DMA(this_hadc, (uint32_t*)&G_ADCBuffer, MY_ADC_BUFFER_LENGTH) != HAL_OK) {
        /* Start Conversation Error */
        Error_Handler();
    }
}

// デバッグ用
// アナログポートに接続したピンをADCした値をシリアルに表示する
uint16_t ADC_GetRawValue(uint8_t i_idx) {
    // 入力された値が、予め設定されている定数より多いと、エラー（-1）を出力
    if (i_idx < MY_ADC_BUFFER_LENGTH) {
        return G_ADCBuffer[i_idx];
    } else {
        dprintUART((uint8_t *)"ADC_GetRawValue() i_idx over num error :", 0);
        return -1;
    }
}

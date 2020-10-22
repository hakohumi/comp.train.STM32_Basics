/*
 * State.c
 *
 *  Created on: 2020/10/20
 *      Author: fuminori.hakoishi
 */

#include "State.h"

#include "Distance.h"
#include "LCD.h"
#include "Temp_ADC.h"
#include "Temp_I2C.h"
#include "UART.h"
#include "mystringfunc.h"

void State_runViewTemp(void);
void State_runViewDistance(void);
void State_runMemDump(void);
void State_runUARTRecieve(void);
void State_runDebugOutput(void);

// mainで宣言している
// システムの状態
static uint8_t SystemState;

// 処理の更新タイミング
bool State_UpdateFlg = false;

// 初期化
void State_Init(uint8_t i_state) {
    SystemState = i_state;
}

// システムの状態遷移
void State_ChangeStateRoll(void) {
    SystemState++;
    if (SystemState >= SYS_STATE_LENGTH) {
        SystemState = 0;
    }

    switch (SystemState) {
        case SYS_STATE_TEMP:
            dprintUART((uint8_t *)"SYS State TEMP : ", SystemState);
            break;
        case SYS_STATE_DISTANCE:
            dprintUART((uint8_t *)"SYS State DISTANCE : ", SystemState);
            break;
        case SYS_STATE_DEBUG_POINTER:
            dprintUART((uint8_t *)"SYS State DEBUG POINTER : ", SystemState);
            break;
        case SYS_STATE_DEBUG_RECIEVE:
            PrintUART((uint8_t *)"SYS State DEBUG RECIEVE\r\n");
            break;
        case SYS_STATE_BLE:
            PrintUART((uint8_t *)"SYS_STATE_BLE\r\n");
            break;
        case SYS_STATE_DEBUG:
            dprintUART((uint8_t *)"SYS State DEBUG : ", SystemState);
            break;
        default:
            PrintUART((uint8_t *)"SYS State main未登録\r\n");
            break;
    }
}

void State_RunProcess(void) {
    // 指定したタイマごとに処理をする
    if (State_UpdateFlg == true) {
        State_UpdateFlg = false;

        /* LCD に書き込み */
        switch (State_GetState()) {
            case SYS_STATE_TEMP:

                State_runViewTemp();

                break;
            case SYS_STATE_DISTANCE:
                State_runViewDistance();
                break;
                // メモリダンプ
            case SYS_STATE_DEBUG_POINTER:
                State_runMemDump();
                break;
                // シリアル送受信
            case SYS_STATE_DEBUG_RECIEVE:
                State_runUARTRecieve();
                break;

            case SYS_STATE_BLE:
                State_runBLE();
                break;
            case SYS_STATE_DEBUG:
                State_runDebugOutput();
                break;
            default:
                break;
        }
    }
}

// リアルタイムで実行される
void State_RunRealtimeProcess(void) {
    switch (State_GetState()) {
        case SYS_STATE_TEMP:
            break;
        case SYS_STATE_DISTANCE:
            break;
        case SYS_STATE_DEBUG_POINTER:
            break;
        case SYS_STATE_DEBUG_RECIEVE:
            UART_ReceiveInput(SystemState);
            break;
        case SYS_STATE_BLE:
            UART_ReceiveInput(SystemState);
            State_runRealtimeBLEInput();
            break;
        case SYS_STATE_DEBUG:
            break;
        default:
            break;
    }
}

void State_runViewTemp(void) {
    // I2C温度センサの温度
    int8_t l_TempI2C = 0;

    // ADC温度センサの温度
    int8_t l_TempADC = 0;

#ifndef NOUSE_I2CTEMP
    // I2C温度センサの温度の取得
    l_TempI2C = TempI2C_GetTemp();
#endif
    // ADCで温度の値を読み取る
    l_TempADC = TEMP_ADC_GetTemp();

    // I2C 温度
    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"I2C:", 4);
    LCD_WriteToBufferInt(4, l_TempI2C, 2);
    // ADC 温度
    LCD_WriteToBuffer(8, (uint8_t *)"ADC:", 4);
    LCD_WriteToBufferInt(12, l_TempADC, 2);
}

void State_runViewDistance(void) {
    // ADC距離センサの距離(cm)
    uint16_t l_DistanceADC = 0;

    // 距離センサの値を読み取る
    l_DistanceADC = Distance_ADC_GetDistance();

    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"DISTANCE", 8);
    LCD_WriteToBufferInt(8, l_DistanceADC, 3);
    LCD_WriteToBuffer(11, (uint8_t *)"cm", 2);
}

void State_runMemDump(void) {
    // アドレスポインタ
    static uint8_t *l_p = 0x00000000;

    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"DBG ADDR", 8);
    LCD_WriteToBufferInt(8, *l_p, 8);
    printUARTHex((uint8_t *)"Pointer Address : ", (uint32_t)l_p, 8);
    printUARTHex((uint8_t *)"Pointer Value : ", (*l_p), 2);

    l_p++;
}
void State_runUARTRecieve(void) {
    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"RECIEVE", 7);
}

void State_runDebugOutput(void) {
    /* 表示処理 */

#ifdef MYDEBUG
    // 動作確認用のカウント
    static uint16_t l_count = 0;
#endif

#ifndef NOUSE_MOVING_AVERAGE_FILTER
    // 平均算出用
    uint16_t l_value;
    static uint16_t l_avrVal = 0;
    static uint8_t l_avrCnt  = 0;
#endif

    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"DEBUG:", 5);

#ifdef MYDEBUG
// シリアルに温度送信
#ifndef NOUSE_I2CTEMP
#ifdef MYDEBUG_I2CTEMP
    dprintUART("i2c tmp : ", TempI2C_GetTemp());
#endif
#endif

#ifdef MYDEBUG_ADCTEMP
    // アナログ温度センサの値を送信
    dprintUART("Tim21over : adc tmp : ", TEMP_ADC_GetTemp());
#endif

// ADCした値をデバッグ
#ifndef NOUSE_MOVING_AVERAGE_FILTER
    dprintUART("ADC exTemp Value :", ADC_GetRawValue(ADC_DATA_IDX_EXTEMP));

    // 距離センサの平均値を求める処理
    l_value = ADC_GetRawValue(ADC_DATA_IDX_DISTANCE);
    l_avrVal += l_value;
    l_avrCnt++;

    dprintUART("ADC Distance sensor Value :", l_value);
    //		printUART("ADC Distance Average  :", l_avrVal);
    if (l_avrCnt > 9) {
        l_avrVal /= l_avrCnt;
        dprintUART("Average cnt : ", l_avrCnt);
        dprintUART("ADC Distance 10 times Average : ", l_avrVal);
        l_avrVal = 0;
        l_avrCnt = 0;
    }

#endif

#ifdef MYDEBUG_DISTANCE
    dprintUART("ADC Distance :", (uint16_t)Distance_ADC_GetDistance());
#endif

    // 内蔵温度センサ
    //		printUART("ADC inTemp Value :", ADC_GetRawValue(ADC_DATA_IDX_INTEMP));
    // 内部電圧値
    //		printUART("ADC VREFINT Value :", ADC_GetRawValue(ADC_DATA_IDX_VREFINT));

    // デバッグ用
    dprintUART((uint8_t *)"Timer :  tim21  overflow cnt : ", (uint16_t)l_count);

    l_count++;

#endif
}

void State_runBLE(void) {
    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"BLE", 3);
}

void State_runRealtimeBLEInput(void) {
    // LINBLEの状態を更新したい
    // リンブル（UART1）から受信があった時、
    // バッファを取りためて、
    // 現在のLINBLEの状態と、受信した文字とを考慮して、
    // 現在のLIBNLEの状態を更新する
}

uint8_t State_GetState(void) {
    return SystemState;
}

void State_SetUpdateFlg(void) {
    State_UpdateFlg = true;
}

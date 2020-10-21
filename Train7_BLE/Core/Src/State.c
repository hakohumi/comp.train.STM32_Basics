/*
 * State.c
 *
 *  Created on: 2020/10/20
 *      Author: fuminori.hakoishi
 */

#include "State.h"

#include "Distance.h"
#include "Dump.h"
#include "LCD.h"
#include "Temp_ADC.h"
#include "Temp_I2C.h"
#include "UART.h"
#include "mystringfunc.h"
#include "LINBLE.h"

void State_runViewTemp(void);
void State_runViewDistance(void);
void State_runMemDump(void);
void State_runUARTRecieve(void);
void State_runDebugOutput(void);
void State_runBLE(void);

// mainで宣言している
// システムの状態
static uint8_t SystemState;
bool BLEreconnectFlg = true;

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
                BLEreconnectFlg = true;

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
    LCD_WriteToBuffer(11,(uint8_t *) "cm", 2);
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

// UARTのほうでリアルタイムで呼ばれる
void State_RunUARTRecieveInUART(void) {
    // 文字列バッファ
    uint8_t l_strBuf[64];
    // 文字列の長さ
    uint8_t l_strLength = 0;
    // 整数型の入力値
    uint32_t l_value = 0;
    // 文字列を16進数に変換した時の返り値
    int8_t l_status;
    // エンター何回目か
    static uint8_t ls_EnterNum = 0;
    // 引数1つ目と2つ目の値を格納する配列
    static uint32_t ls_FuncArgumentArray[2] = {0, 0};

    l_strLength = UART_GetReceiveData(&l_strBuf, 64);
    PrintUARTInt(l_strLength);

    if (l_strLength == 1) {
        if (l_strBuf[0] == '\0') {
            // enterの文字と、受信した文字列を表示
            PrintUART((uint8_t *)"enter\r\n");
        }
    } else {
        PrintUART((uint8_t *)"Input : ");
        PrintUART(l_strBuf);
        PrintUART((uint8_t *)"\r\n");
    }

    // 入力された文字がコマンド(アドレス)かどうか
    // 文字数チェック
    if (l_strLength > 9) {
        PrintERROR(ERROR_INPUT_OVERLENGTH);

        goto INPUT_ERROR;
    } else if (l_strLength == 1) {
        // もし1文字目でエンターを押した場合
        // 改行を表示するだけ
        PrintERROR(ERROR_INPUT_SHORTLENGTH);

        goto INPUT_ERROR;
    }

    // 文字列を16進数に変換
    l_status = MyString_Atoi(&l_value, l_strBuf, l_strLength);
    dprintUART("MyString_Atoi : ", l_value);

    // 1要素目（開始アドレスの指定）
    if (ls_EnterNum == 0) {
        if (l_status == -1) {
            goto INPUT_ERROR;
        } else {
            ls_FuncArgumentArray[0] = l_value;
        }

        ls_EnterNum = 1;
        // 2要素目（読み取るサイズ）
    } else if (ls_EnterNum == 1) {
        if (l_status == -1) {
            goto INPUT_ERROR;
        } else {
            ls_FuncArgumentArray[1] = l_value;
        }

        Dump_sendMemDumpUART((uint8_t *)ls_FuncArgumentArray[0],
            ls_FuncArgumentArray[1]);

    INPUT_ERROR:
        ls_EnterNum             = 0;
        ls_FuncArgumentArray[0] = 0;
        ls_FuncArgumentArray[1] = 0;
    }
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
    LCD_WriteToBuffer(0,(uint8_t *) "DEBUG:", 5);

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

typedef enum {
    LINBLE_STATE_COMMAND,
    LINBLE_STATE_ADVERTISE,
    LINBLE_STATE_ONLINE,
} LINBLE_State_Type;

void State_runBLE(void) {
    static int8_t l_LINBLEStatus = LINBLE_STATE_COMMAND;

    int8_t l_retMesg = 0;
    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"BLE", 3);

    switch (l_LINBLEStatus) {
        case LINBLE_STATE_COMMAND:

            // コマンド状態から、アドバタイズ状態へ遷移させる
            // BTA<CR>コマンドを送信する
            if (BLEreconnectFlg == true) {
                l_retMesg = LINBLE_StartConnection();
                if (l_retMesg != 0) {
                    PrintUART((uint8_t *)"State_runBLE error\r\n");
                } else {
                    // アドバタイズ状態へ遷移
                    l_LINBLEStatus = LINBLE_STATE_ADVERTISE;
                }
                BLEreconnectFlg = false;
            }
            break;
        case LINBLE_STATE_ADVERTISE:
            PrintUART((uint8_t *)"アドバタイズ状態です。\r\n");
            break;

        case LINBLE_STATE_ONLINE:
            PrintUART((uint8_t *)"オンライン状態です。\r\n");
            break;
        default:
            PrintUART((uint8_t *)"Error runBLE : switch default reached.\r\n");
            break;
    }

    // アドバタイズ状態から接続に成功すると、"CONN<CR><LF>"が返ってくる。
}

uint8_t State_GetState(void) {
    return SystemState;
}

void State_SetUpdateFlg(void) {
    State_UpdateFlg = true;
}

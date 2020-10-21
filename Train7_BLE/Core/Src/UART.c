/*
 * UART.c
 *
 *  Created on: Sep 29, 2020
 *      Author: fuminori.hakoishi
 */
#include "UART.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "Dump.h"
#include "main.h"

#define DATANUM 128

/*
 * The STM32 makes receiving chars into a large circular buffer simple
 * and requires no CPU time. The UART receiver DMA must be setup as CIRCULAR.
 */

#define UART_RECEIVE_BUF 64
#define INPUT_BUF_SIZE 64

static uint8_t UART_ReceiveData[UART_RECEIVE_BUF];

// 入力された1行の文字数
static uint8_t UART_ReceiveCount;
static bool UART_ReceiveCountOverFlowFlg = false;

static UART_HandleTypeDef *l_huart;

// 受信フラグ
bool UART_ReceiveFlg = false;
// Enterフラグ
bool UART_ReceiveEnterFlg = false;

// 受信したデータを格納する
void UART_SetReceiveData(uint8_t i_data) {
    // エンターを押された時
    if (i_data == '\r' || i_data == '\n') {
        UART_ReceiveEnterFlg = true;
    }

    // 受信したデータを格納
    UART_ReceiveData[UART_ReceiveCount] = i_data;

    // 受信フラグ ON
    UART_ReceiveFlg = true;

    // 入力桁数を増加
    UART_ReceiveCount++;
    // 入力がバッファを超えたら、
    if (UART_ReceiveCount >= UART_RECEIVE_BUF) {
        // バッファを最初からにする
        UART_ReceiveCount            = 0;
        UART_ReceiveCountOverFlowFlg = true;
    }
}

// -----------------------------------------

// 初期化
void UART_Console_Init(UART_HandleTypeDef *huart) {
    l_huart = huart;

    HAL_UART_Receive_IT(l_huart, &UART_ReceiveData, 1);
}

// 新入力処理
void UART_ReceiveInput(void) {
    // とりあえず、Enter押したら、1行が表示されるようにする
    // 前提：メインでずっと処理される

    // エンターフラグが立っていたら
    if (UART_ReceiveEnterFlg == true) {
    }
}

// バッファに入っているデータを取得する
uint8_t *UART_GetReceiveData(uint8_t *o_strAddr) {
    uint8_t i = 0;

    while (UART_ReceiveData[i] != '\0') {
        *o_strAddr = UART_ReceiveData[i];
        i++;
    }

    // バッファを先頭に戻す
    UART_ReceiveCount = 0;

    return i;
}

// 入力受付処理
// いずれはこれを、UARTからエンターが押された時に1行リードする関数としたい
void UART_RecieveInput(void) {
    // UART入力用バッファ
    static uint8_t InputBufArray[INPUT_BUF_SIZE];
    // UART入力バッファ用インデックス
    static uint8_t InputBufIdx = 0;

    /* ----------- シリアルの処理 ------------ */

    // 値が確定（エンターが押された、バッファを超えた）した時に、フラグが立つ
    // 立っていると、処理の最後にインデックスを0に戻す
    // 立っていない時は、処理の最後にインデックスをカウントアップする
    // 1文字表示用バッファ
    uint8_t l_UART_StrBufChar[2];

    bool l_UART_BeginIdx      = false;
    static uint8_t l_UART_Len = 0;
    // 引数1つ目と2つ目の値を格納する配列
    static uint32_t l_UART_ArgumentArray[2] = {0, 0};

    if (UART_ReceiveFlg == true) {
        UART_ReceiveFlg == false;
        // InputBufArray[InputBufIdx] = msgrx_circ_buf_get();

        // 入力した文字をシリアルに送信
        l_UART_StrBufChar[0] = InputBufArray[InputBufIdx];
        l_UART_StrBufChar[1] = '\0';
        PrintUARTn(l_UART_StrBufChar, 2);

        // バッファ分まで入力が溜まった時
        if (InputBufIdx > INPUT_BUF_SIZE - 1) {
            InputBufArray[InputBufIdx] = '\0';
            PrintUARTn(InputBufArray, 2);
            PrintUART("\r\n");
            l_UART_BeginIdx = true;
        }

        // エンターを押された時
        if (InputBufArray[InputBufIdx] == '\r' || InputBufArray[InputBufIdx] == '\n') {
            InputBufArray[InputBufIdx] = '\0';

#ifdef MYDEBUG
            dprintUART("l_UART_LEN: ", l_UART_Len);
#endif

            PrintUART("Input : ");
            if (InputBufIdx == 0) {
                // enterの文字と、受信した文字列を表示
                PrintUART("enter\r\n");
            } else {
                PrintUART(InputBufArray);
                PrintUART("\r\n");
            }

            // 入力された文字がコマンド(アドレス)かどうか
            // 文字数チェック
            if (InputBufIdx > 8) {
                PrintERROR(ERROR_INPUT_OVERLENGTH);

                goto INPUT_ERROR;
            } else if (InputBufIdx == 0) {
                // もし1文字目でエンターを押した場合
                // 改行を表示するだけ
                PrintERROR(ERROR_INPUT_SHORTLENGTH);

                goto INPUT_ERROR;
            }

            uint32_t l_value = 0;
            int8_t l_status  = MyString_Atoi(&l_value, InputBufArray, InputBufIdx);
            dprintUART("MyString_Atoi : ", l_value);

            /*   ここまでで、1回分の文字列を数値に変換は完了 */

            // 1要素目（開始アドレスの指定）
            if (l_UART_Len == 0) {
                if (l_status == -1) {
                    goto INPUT_ERROR;
                } else {
                    l_UART_ArgumentArray[l_UART_Len] = l_value;
                }

                l_UART_BeginIdx = true;
                l_UART_Len++;
                // 2要素目（読み取るサイズ）
            } else if (l_UART_Len == 1) {
                if (l_status == -1) {
                    goto INPUT_ERROR;
                } else {
                    l_UART_ArgumentArray[l_UART_Len] = l_value;
                }

                Dump_sendMemDumpUART((uint8_t *)l_UART_ArgumentArray[0],
                    l_UART_ArgumentArray[1]);

            // gotoラベル 例外処理
            INPUT_ERROR:

                l_UART_BeginIdx         = true;
                l_UART_ArgumentArray[0] = 0;

                l_UART_Len              = 0;
                l_UART_ArgumentArray[1] = 0;
            }
        }  // エンター押された時の処理 終了括弧

        // 入力された値が確定しているか
        // していたらインデックスの初期化
        if (l_UART_BeginIdx == true) {
            l_UART_BeginIdx = false;
            InputBufIdx     = 0;
        } else {
            InputBufIdx++;
        }
    }
}

// UARTに文字列を表示
// NULL終端前提
// チェック：
//      NULL
//		文字数
//　終端
#define BUF_STR_SIZE 128
int8_t PrintUART(uint8_t *i_str) {
    /* ----------------------- エラー処理 -------------------*/
    // 文字数、終端文字
    /* ---------------------------------------------------- */

    // 終端文字を見つけるまで かつ バッファ分まで
    if (MyString_FindEOL(i_str, BUF_STR_SIZE) <= 0) {
        PrintERROR(ERROR_UART_PRINTUART_ENDOFLINE);
        return -1;
    }

    /* -------------------------------------*/

    //	int status = HAL_UART_Transmit_DMA(huart_cobs, i_str,
    //			(uint16_t) (strlen((const char*) i_str)));
    int status = HAL_UART_Transmit(l_huart, i_str, (uint16_t)(strlen((const char *)i_str)), 0xffff);
    //    int status = HAL_UART_Transmit_IT(huart_cobs, i_str, (uint16_t)(strlen((const char *)i_str)));
    return status == HAL_OK;

    // 1バイトずつ送る
    // while (*i_str != '\0') {
    // 	HAL_UART_Transmit(huart_cobs, i_str, 1, 0xffff);
    // 	i_str++;
    // }

    // return 0;
}

int8_t PrintUARTn(uint8_t *i_str, uint8_t i_size) {
    /* ----------------------- エラー処理 -------------------*/
    // 文字数、終端文字
    /* ---------------------------------------------------- */

    // 終端文字を見つけるまで かつ バッファ分まで
    if (MyString_FindEOL(i_str, i_size) <= 0) {
        i_str[i_size] = '\0';
    }

    /* -------------------------------------*/

    int status = HAL_UART_Transmit(l_huart, i_str, (uint16_t)(strlen((const char *)i_str)), 0xffff);
    return status == HAL_OK;
}

// 数値のみを表示
int8_t PrintUARTInt(uint32_t i_var) {
    uint8_t l_buffer[64];
    sprintf((char *)&l_buffer, "%ld\r\n", i_var);

    int status = HAL_UART_Transmit(l_huart, l_buffer, (uint16_t)strlen((const char *)&l_buffer), 0xffff);
    return status == HAL_OK;
}

// デバッグ用
bool dprintUART(uint8_t *i_str, uint32_t i_var) {
    uint8_t l_buffer[64];
    uint8_t l_buffer2[64];

    sprintf((char *)&l_buffer, "%s%ld", i_str, i_var);

    sprintf((char *)&l_buffer2, "%s, buf size : %d\r\n", (char *)&l_buffer, strlen((const char *)&l_buffer));

    //	int status = HAL_UART_Transmit_DMA(huart_cobs, l_buffer2,
    //			(uint16_t) (strlen((const char*) &l_buffer2)));
    int status = HAL_UART_Transmit(l_huart, l_buffer2, (uint16_t)(strlen((const char *)&l_buffer2)), 0xffff);
    return status == HAL_OK;
}

// 16進で表示
// i_len は桁数
bool printUARTHex(uint8_t *i_str, uint32_t i_var, uint8_t i_len) {
    uint8_t l_buffer[64];

    if (i_len == 4) {
        sprintf((char *)&l_buffer, "%s0x%04x\r\n", i_str, i_var);
    } else if (i_len == 8) {
        sprintf((char *)&l_buffer, "%s0x%08x\r\n", i_str, i_var);
    } else if (i_len == 2) {
        sprintf((char *)&l_buffer, "%s0x%02x\r\n", i_str, i_var);
    } else {
        sprintf((char *)&l_buffer, "%s0x%x\r\n", i_str, i_var);
    }

    //	int status = HAL_UART_Transmit_DMA(huart_cobs, l_buffer,
    //			(uint16_t) strlen((const char*) &l_buffer));
    int status = HAL_UART_Transmit(l_huart, l_buffer, (uint16_t)strlen((const char *)&l_buffer), 0xffff);
    return status == HAL_OK;
}

void PrintERROR(uint8_t i_errorCode) {
    switch (i_errorCode) {
        case ERROR_INPUT_OVERLENGTH:
            PrintUART("ERROR INPUT OVERLENGTH\r\n");
            break;
        case ERROR_INPUT_SHORTLENGTH:
            PrintUART("ERROR_INPUT_SHORTLENGTH\r\n");
            break;
        case ERROR_INPUT_CONTROL_CODE:
            PrintUART("ERROR_INPUT_CONTROL_CODE\r\n");
            break;
        case ERROR_INPUT_ALPHANUMERIC:
            PrintUART("ERROR_INPUT_ALPHANUMERIC\r\n");
            break;
        case ERROR_INPUT_HEX:
            PrintUART("ERROR_INPUT_HEX\r\n");
            break;
        case ERROR_UART_PRINTUART_ENDOFLINE:
            PrintUART("ERROR_UART_PRINTUART_ENDOFLINE\r\n");
            break;
        case ERROR_LINBLE_SENDFAILURE:
            PrintUART("ERROR_LINBLE_SENDFAILURE\r\n");
            break;
        case ERROR_LINBLE_NOTFIND_ENDOFLINE:
            PrintUART("ERROR_LINBLE_NOTFIND_ENDOFLINE\r\n");
            break;
        case ERROR_LINBLE_NOTFIND_CR:
            PrintUART("ERROR_LINBLE_NOTFIND_CR\r\n");
            break;
        case ERROR_MYSTRINGFUNC_NOTFIND_ENDOFLINE:
            PrintUART("ERROR_MYSTRINGFUNC_NOTFIND_ENDOFLINE\r\n");
            break;

        case ERROR_LINBLE_RECIEVEFAILURE:
            PrintUART("ERROR_LINBLE_RECIEVEFAILURE\r\n");
            break;

        default:
            PrintUART("Unregistered error\r\n");
            break;
    }
}

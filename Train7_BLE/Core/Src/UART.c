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
#include "State.h"

#define DATANUM 128

/*
 * The STM32 makes receiving chars into a large circular buffer simple
 * and requires no CPU time. The UART receiver DMA must be setup as CIRCULAR.
 */

#define UART_RECEIVE_BUF 64
#define INPUT_BUF_SIZE 64

// バッファ
static uint8_t UART_ReceiveData1[UART_RECEIVE_BUF];
// static uint8_t UART_ReceiveData2[UART_RECEIVE_BUF];

// 最後の文字のバッファ
static uint8_t UART_ReceiveCharLast;

// バッファの使用先
// static uint8_t UART_ReceiveDataIdx = 0;

static bool UART_ReceiveLockFlg = false;

// 入力された1行の文字数
static uint8_t UART_ReceiveCount;
// 最後のカウント
static uint8_t UART_ReceiveCountLast;
static bool UART_ReceiveCountOverFlowFlg = false;

static UART_HandleTypeDef *this_huart;

// 状態
static uint8_t UART_State = UART_STATE_NONPUSHED;

// 受信フラグ
bool UART_ReceiveLineFlg = false;
bool UART_ReceiveFlg     = false;
// Enterフラグ
bool UART_ReceiveEnterFlg = false;

// バッファが最初に戻ったフラグ
bool UART_ReceiveBufEndFlg = false;

// 受信したデータを格納する
void UART_SetReceiveData(void) {
    uint8_t i_data = UART_ReceiveCharLast;

    // バッファからデータを取り出していない間、受信を受け付けない
    // エンターを押された時
    if (UART_ReceiveLockFlg == false) {
        if (i_data == '\r' || i_data == '\n') {
            UART_ReceiveEnterFlg = true;

            UART_ReceiveLockFlg = true;
            i_data              = '\0';
        }

        // 受信したデータを格納
        UART_ReceiveData1[UART_ReceiveCount] = i_data;

        // 前回の位置を記録
        UART_ReceiveCountLast = UART_ReceiveCount;

        // 入力桁数を増加
        UART_ReceiveCount++;

        // 入力がバッファを超えたら、
        if (UART_ReceiveCount >= UART_RECEIVE_BUF) {
            // バッファオーバーフラグを立てる
            UART_ReceiveCountOverFlowFlg = true;

            // 最後に終端文字を入れる
            UART_ReceiveData1[UART_RECEIVE_BUF] = '\0';

            // バッファを最初からにする
            UART_ReceiveCount = 0;
        }

        if (UART_ReceiveEnterFlg == true) {
            UART_ReceiveEnterFlg = false;
            UART_ReceiveCount    = 0;
        }

        // バッファの最初に戻ったかどうか
        if (UART_ReceiveEnterFlg || UART_ReceiveCountOverFlowFlg) {
            // バッファの終わりフラグを立てる
            UART_ReceiveBufEndFlg = true;
            // 1行受信フラグ ON
            UART_ReceiveLineFlg = true;
        }

        UART_ReceiveFlg = true;
    }
    HAL_UART_Receive_IT(this_huart, &UART_ReceiveCharLast, 1);
}

uint8_t UART_GetState(void) {
    return UART_State;
}

uint8_t UART_GetReceiveLineFlg(void) {
    return UART_ReceiveLineFlg;
}

void UART_ClrReceiveLineFlg(void) {
    UART_ReceiveLineFlg = false;
}

// -----------------------------------------

// 初期化
void UART_Console_Init(UART_HandleTypeDef *huart) {
    this_huart = huart;

    HAL_UART_Receive_IT(this_huart, &UART_ReceiveCharLast, 1);
}

// 新入力処理
uint8_t UART_ReceiveInput(uint8_t i_state) {
#ifdef MYDEBUG
    uint8_t l_buf[UART_RECEIVE_BUF];
#endif
    UART_State = UART_STATE_NONPUSHED;

    switch (i_state) {
        case SYS_STATE_DEBUG_RECIEVE:

            // とりあえず、Enter押したら、1行が表示されるようにする
            // 前提：メインでずっと処理される

            // バッファビジーフラグが立っていたら
            if (UART_ReceiveLockFlg == true) {
                UART_State = UART_STATE_PUSHED_ENTER;
#ifdef MYDEBUG
                UART_GetReceiveData(&l_buf, UART_RECEIVE_BUF);
                PrintUART("DEBUG:UART_ReceiveInput() : ");
                PrintUART(l_buf);
                PrintUART("\r\n");
#endif

                State_RunUARTRecieveInUART();
                UART_ReceiveLockFlg = false;
            }

            break;

        default:
            break;
    }

    return UART_State;
}

// 最後に入力された文字を返す
uint8_t UART_GetReceiveCharLast(void) {
    return UART_ReceiveCharLast;
}

// バッファに入っているデータを取得する
uint8_t UART_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize) {
    uint8_t i = 0;

    while (i <= UART_ReceiveCountLast && i < i_bufSize) {
        *o_strAddr = UART_ReceiveData1[i];
        i++;
        o_strAddr++;
    }

    return i;
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

    int status = HAL_UART_Transmit(this_huart, i_str, (uint16_t)(strlen((const char *)i_str)), 0xffff);
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
    uint8_t l_strBuf[i_size + 1];
    uint8_t i = 0;

    while (i < i_size) {
        l_strBuf[i] = *i_str;
        i_str++;
        i++;
    }
    l_strBuf[i] = '\0';

    /* ----------------------- エラー処理 -------------------*/
    // 文字数、終端文字
    /* ---------------------------------------------------- */

    // 終端文字を見つけるまで かつ バッファ分まで
    if (MyString_FindEOL(&l_strBuf, i_size + 1) <= 0) {
        l_strBuf[i_size] = '\0';
    }

    /* -------------------------------------*/

    int status = HAL_UART_Transmit(this_huart, &l_strBuf, (uint16_t)(strlen((const char *)&l_strBuf)), 0xffff);
    return status == HAL_OK;
}

void PrintChar(uint8_t i_char) {
    int status;
    uint8_t l_buf[2];

    if (MyString_CheckCharCtrlCode(i_char) == true) {
        l_buf[0] = i_char;
        l_buf[1] = 0;
        status   = HAL_UART_Transmit(this_huart, (uint8_t *)l_buf, (uint16_t)2, 0xffff);
    } else {
        status = HAL_UART_Transmit(this_huart, (uint8_t *)"  ", (uint16_t)2, 0xffff);
    }
    return status == HAL_OK;
}

// 数値のみを表示
int8_t PrintUARTInt(uint32_t i_var) {
    uint8_t l_buffer[64];
    sprintf((char *)&l_buffer, "%ld\r\n", i_var);

    int status = HAL_UART_Transmit(this_huart, l_buffer, (uint16_t)strlen((const char *)&l_buffer), 0xffff);
    return status == HAL_OK;
}

// デバッグ用
bool dprintUART(uint8_t *i_str, uint32_t i_var) {
    uint8_t l_buffer[64];
    uint8_t l_buffer2[64];

    sprintf((char *)&l_buffer, "%s%ld", i_str, i_var);

    sprintf((char *)&l_buffer2, "%s, buf size : %d\r\n", (char *)&l_buffer, strlen((const char *)&l_buffer));

    int status = HAL_UART_Transmit(this_huart, l_buffer2, (uint16_t)(strlen((const char *)&l_buffer2)), 0xffff);
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

    int status = HAL_UART_Transmit(this_huart, l_buffer, (uint16_t)strlen((const char *)&l_buffer), 0xffff);
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

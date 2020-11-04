/*
 * UART.c
 *
 *  Created on: Sep 29, 2020
 *      Author: fuminori.hakoishi
 */

/*

このファイルは、主にPCとマイコンとのUART通信に使用する関数をまとめたものである
UART自体の処理はHALライブラリを使用している
UARTは1つのマイコンに1つ以上存在するため、ある程度共通化できる部分は共通化したい

*/

#include "UART.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "Dump.h"
#include "LINBLE.h"
#include "State.h"
#include "mystringfunc.h"

#define DATANUM 128

#define UART_TRANSMIT_TIMEOUT 0xffff

/*
 * The STM32 makes receiving chars into a large circular buffer simple
 * and requires no CPU time. The UART receiver DMA must be setup as CIRCULAR.
 */

#define UART_RECEIVE_BUF 64
#define INPUT_BUF_SIZE 64

void UART_enterHundler(uint8_t i_sysState);

// バッファ
static uint8_t UART_ReceiveData1[UART_RECEIVE_BUF];
// static uint8_t UART_ReceiveData2[UART_RECEIVE_BUF];

// 最後の文字のバッファ
static uint8_t UART_ReceiveCharLast = 0;

// バッファの使用先
// static uint8_t UART_ReceiveDataIdx = 0;

// 入力された1行の文字数
static uint8_t UART_ReceiveCount;
// 最後のカウント
static uint8_t UART_ReceiveCountLast;

static UART_HandleTypeDef *this_huart;

// 状態
static uint8_t UART_State = UART_STATE_NONPUSHED;

// 受信フラグ
bool UART_ReceiveLineFlg = false;
bool UART_ReceiveFlg     = false;

// バッファが最初に戻ったフラグ
bool UART_ReceiveBufEndFlg = false;

// terattermからの受信
// キー入力による文字列が入力される
// エンターが環境によって違う
// CRもLFもCR + LFもどれもCR + LFにする
// CRかLFを受信した場合、CR + LFに直す
// 受信したデータを格納する
void UART_SetReceiveData(void) {
    // Enterフラグ
    // SetReceiveData()内のフラグ
    bool l_receiveEnterFlg = false;
    // バッファオーバーフローフラグ
    bool l_receiveCountOverFlowFlg = false;

    uint8_t i_data    = UART_ReceiveCharLast;
    static bool CRFlg = false;
    // LFフラグ
    bool l_LFFlg = false;

#define MYDEBUG_UART_SETRECEIVEDATA
#ifdef MYDEBUG_UART_SETRECEIVEDATA

    // 改行コードの判定

    // CRのみの時
    // ① CRかどうか比較して、trueならLFを追加する
    // ② この時にすでに改行したことにし、次のLFの入力があった場合、無効にする

    // LFのみの時
    // ③ CRがなかった場合、LFかどうかを比較して、trueなら終端を追加する
    // falseなら、そのまま受信データをバッファに格納する

    // CRが来た後、次の1バイトがLFかどうかが知りたい
    // trueならスルー
    // falseならそのまま

    // CRフラグ 検知前
    if (CRFlg == false) {
        if (i_data == '\r') {
            // 次にLFが来てもスルーする用にCRフラグを立てる
            CRFlg = true;
        }

        if ((CRFlg == true) || (i_data == '\n')) {
            // 現在の位置にCRを追加
            // receiveCount
            // バッファの最後
            // UART_ReceiveData1[61] = '\r'
            UART_ReceiveData1[UART_ReceiveCount] = '\r';

            // UART_ReceiveCount = 62
            UART_ReceiveCount++;

            // 次の位置にLFを追加
            // receiveCount + 1
            // UART_ReceiveData1[62] = '\n'
            UART_ReceiveData1[UART_ReceiveCount] = '\n';

            // UART_ReceiveCount = 63
            UART_ReceiveCount++;

            // そのさらに次の位置に終端を追加
            // receiveCoun + 2
            UART_ReceiveData1[UART_ReceiveCount] = '\0';

            l_receiveEnterFlg = true;
        }

    } else {  // CRフラグ 検知後 CRの次のバイト

        // CRの次にLFが来た場合
        if (i_data == '\n') {
            // 今回の割込みは全スルーする
            // CRフラグを下げる

            PrintUART((uint8_t *)"2文字目のLFはスルーしたよ！\r\n");
            UART_ReceiveCharLast = '\r';
            l_LFFlg              = true;
        } else {
            if (i_data == '\r') {
                UART_ReceiveData1[UART_ReceiveCount] = '\r';

                // UART_ReceiveCount = 62
                UART_ReceiveCount++;
                l_receiveEnterFlg = true;
            }
        }

        // CRフラグを下げる
        CRFlg = false;
    }

#endif

    if (l_LFFlg == false) {
        // 入力が改行ではない場合
        if (l_receiveEnterFlg == false) {
            // 受信したデータを格納
            UART_ReceiveData1[UART_ReceiveCount] = i_data;
        }

        // 前回の位置を記録
        UART_ReceiveCountLast = UART_ReceiveCount;

        // 入力桁数を増加
        UART_ReceiveCount++;

        // 入力がバッファを超えたら、
        // BufSize = 64
        // buf[63] = '\0'
        // buf[62] = '\n'
        // buf[61] = '\r'

        // 62 になったら、バッファを最初からにする。
        if (UART_ReceiveCount >= UART_RECEIVE_BUF - 2) {
            // バッファオーバーフラグを立てる
            l_receiveCountOverFlowFlg = true;

            // 最後に終端文字を入れる
            // UART_ReceiveData1[62] = '\0'
            UART_ReceiveData1[UART_ReceiveCount] = '\0';
            // UART_ReceiveData1[63] = '\0'
            UART_ReceiveData1[UART_ReceiveCount + 1] = '\0';

            // バッファを最初からにする
            UART_ReceiveCount = 0;
        }

        // バッファの最初に戻ったかどうか
        if (l_receiveEnterFlg || l_receiveCountOverFlowFlg) {
            // バッファの終わりフラグを立てる
            UART_ReceiveBufEndFlg = true;
            // 1行受信フラグ ON
            UART_ReceiveLineFlg = true;
        }

        // エンターでバッファが最初からに戻る処理
        //状態もEnterが押された状態へ遷移する
        if (l_receiveEnterFlg == true) {
            UART_State = UART_STATE_PUSHED_ENTER;
            // UART_ReceiveData1[UART_ReceiveCount] = '\0';
            UART_ReceiveCount = 0;
        }

        UART_ReceiveFlg = true;
    }
}

// 割込みの再設定
// mainの受信割込みで呼ばれる
void UART_ReloadReceiveInterrupt(void) {
    HAL_UART_Receive_IT(this_huart, &UART_ReceiveCharLast, 1);
}

// 新入力処理
void UART_ReceiveInput(uint8_t i_sysState) {
#ifdef MYDEBUG_UART_RECEIVEINPUT
    uint8_t l_buf[UART_RECEIVE_BUF];
#endif

    // とりあえず、Enter押したら、1行が表示されるようにする
    // 前提：メインでずっと処理される

    if (UART_ReceiveFlg == true) {
        if (UART_State == UART_STATE_NONPUSHED) {
            // 入力を表示
            PrintChar(UART_GetReceiveCharLast());

        } else if (UART_State ==
                   UART_STATE_PUSHED_ENTER) {  // エンターが押された時の処理
            // Enterを押したら、改行させる
            PrintUART((uint8_t *)"\r\n");

// #define MYDEBUG_UART_RECEIVEINPUT
#ifdef MYDEBUG_UART_RECEIVEINPUT
            UART_GetReceiveData(&l_buf, UART_RECEIVE_BUF);
            PrintUART((uint8_t *)"DEBUG:UART_ReceiveInput() : ");
            PrintUART(l_buf);
#endif

            UART_enterHundler(i_sysState);
            LINBLE_EnterHandler(i_sysState);

            UART_State = UART_STATE_NONPUSHED;
        }

        UART_ReceiveFlg = false;
    }
}

void UART_enterHundler(uint8_t i_sysState) {
    switch (i_sysState) {
        case SYS_STATE_DEBUG_RECIEVE:
            UART_RunMemDump();
            break;
        default:
            // 何もしない
            break;
    }
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

// 最後に入力された文字を返す
uint8_t UART_GetReceiveCharLast(void) {
    return UART_ReceiveCharLast;
}

// バッファに入っているデータを取得する
uint8_t UART_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize) {
    uint8_t i            = 0;
    uint8_t l_CRLFEOFNum = 0;

    // バッファサイズより大きい場合の例外
    if (UART_ReceiveCountLast > i_bufSize) {
        PrintUART((uint8_t *)"error uart getreceivedata \r\n");
        return 0;
    }

    while (i <= UART_ReceiveCountLast && i < i_bufSize) {
        *o_strAddr = UART_ReceiveData1[i];
        // 改行、終端文字の場合は、カウントしない
        // if (*o_strAddr == '\r' || *o_strAddr == '\n' || *o_strAddr == '\0') {
        if (*o_strAddr == '\0') {
            l_CRLFEOFNum++;
        }

#ifdef MYDEBUG_UART_GETRECEIVEDATA
        PrintChar(*o_strAddr);
#endif
        i++;
        o_strAddr++;
    }

    return i - l_CRLFEOFNum;
}

// エンターが押された時の処理
// メモリダンプ処理
void UART_RunMemDump(void) {
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

    l_strLength = UART_GetReceiveData((uint8_t *)&l_strBuf, 64);
    PrintUARTInt(l_strLength);

    if (l_strLength == 1) {
        if (l_strBuf[0] == '\r' || l_strBuf[0] == '\n') {
            // enterの文字と、受信した文字列を表示
            PrintUART((uint8_t *)"enter\r\n");
        }
    } else {
        PrintUART((uint8_t *)"Input : ");
        PrintUARTn(l_strBuf, 64);
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
    PrintUART((uint8_t *)"MyString_Atoi : ");
    PrintUARTInt(l_value);

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

// UARTに文字列を表示
// NULL終端前提
// チェック：
//      NULL
//		文字数
//　終端
#define BUF_STR_SIZE 128

int8_t PrintUART(uint8_t *i_str) {
    uint8_t l_strLength = 0;
    /* ----------------------- エラー処理 -------------------*/
    // 文字数、終端文字
    /* ---------------------------------------------------- */
    // 終端文字か改行文字を見つけるまで かつ バッファ分まで
    l_strLength = MyString_FindEOL(i_str, BUF_STR_SIZE);
    if (l_strLength < 0) {
        l_strLength = MyString_FindLF(i_str, BUF_STR_SIZE);
        if (l_strLength < 0) {
            l_strLength = MyString_FindCR(i_str, BUF_STR_SIZE);
            if (l_strLength < 0) {
                PrintERROR(ERROR_UART_PRINTUART_ENDOFLINE);
                return -1;
            }
        } else if (l_strLength == 0) {
            PrintUART((uint8_t *)"LFが1文字目です。\r\n");
            // return -1;
        }
    } else if (l_strLength == 0) {
        PrintUART((uint8_t *)"終端文字が1文字目です。\r\n");
        // return -1;
    }

    l_strLength++;

    /* -------------------------------------*/

    int status = HAL_UART_Transmit(this_huart, i_str, (uint16_t)(strlen((const char *)i_str)), UART_TRANSMIT_TIMEOUT);
    return status == HAL_OK;
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
    if (MyString_FindEOL((uint8_t *)&l_strBuf, i_size + 1) <= 0) {
        l_strBuf[i_size] = '\0';
    }

    /* -------------------------------------*/

    int status = HAL_UART_Transmit(this_huart, (uint8_t *)&l_strBuf, (uint16_t)(strlen((const char *)&l_strBuf)), UART_TRANSMIT_TIMEOUT);
    return status == HAL_OK;
}

uint8_t PrintChar(uint8_t i_char) {
    int status = HAL_ERROR;
    uint8_t l_buf[2];

    l_buf[0] = i_char;
    l_buf[1] = 0;

    if (MyString_CheckCharCtrlCode(i_char) == true) {
        status = HAL_UART_Transmit(this_huart, (uint8_t *)l_buf, (uint16_t)2, UART_TRANSMIT_TIMEOUT);
    } else if (i_char == '\r' || i_char == '\n') {
        status = HAL_UART_Transmit(this_huart, (uint8_t *)l_buf, (uint16_t)2, UART_TRANSMIT_TIMEOUT);
    } else if (i_char == '\0') {
        status = HAL_OK;
    } else {
        l_buf[0] = '.';
        status   = HAL_UART_Transmit(this_huart, (uint8_t *)l_buf, (uint16_t)2, UART_TRANSMIT_TIMEOUT);
    }
    return status == HAL_OK;
}

// 数値のみを表示
int8_t PrintUARTInt(uint32_t i_var) {
    uint8_t l_buffer[64];
    sprintf((char *)&l_buffer, "%ld\r\n", i_var);

    int status = HAL_UART_Transmit(this_huart, l_buffer, (uint16_t)strlen((const char *)&l_buffer), UART_TRANSMIT_TIMEOUT);
    return status == HAL_OK;
}

// デバッグ用
bool dprintUART(uint8_t *i_str, uint32_t i_var) {
    uint8_t l_buffer[64];
    uint8_t l_buffer2[64];

    sprintf((char *)&l_buffer, "%s%ld", i_str, i_var);

    sprintf((char *)&l_buffer2, "%s, buf size : %d\r\n", (char *)&l_buffer, strlen((const char *)&l_buffer));

    int status = HAL_UART_Transmit(this_huart, l_buffer2, (uint16_t)(strlen((const char *)&l_buffer2)), UART_TRANSMIT_TIMEOUT);
    return status == HAL_OK;
}

// 16進で表示
// i_len は桁数
bool printUARTHex(uint8_t *i_str, uint32_t i_var, uint8_t i_len) {
    uint8_t l_buffer[64];

    if (i_len == 4) {
        sprintf((char *)&l_buffer, "%s0x%04x\r\n", i_str, (unsigned int)i_var);
    } else if (i_len == 8) {
        sprintf((char *)&l_buffer, "%s0x%08x\r\n", i_str, (unsigned int)i_var);
    } else if (i_len == 2) {
        sprintf((char *)&l_buffer, "%s0x%02x\r\n", i_str, (unsigned int)i_var);
    } else {
        sprintf((char *)&l_buffer, "%s0x%x\r\n", i_str, (unsigned int)i_var);
    }

    int status = HAL_UART_Transmit(this_huart, l_buffer, (uint16_t)strlen((const char *)&l_buffer), UART_TRANSMIT_TIMEOUT);
    return status == HAL_OK;
}

void PrintERROR(uint8_t i_errorCode) {
    switch (i_errorCode) {
        case ERROR_INPUT_OVERLENGTH:
            PrintUART((uint8_t *)"ERROR INPUT OVERLENGTH\r\n");
            break;
        case ERROR_INPUT_SHORTLENGTH:
            PrintUART((uint8_t *)"ERROR_INPUT_SHORTLENGTH\r\n");
            break;
        case ERROR_INPUT_CONTROL_CODE:
            PrintUART((uint8_t *)"ERROR_INPUT_CONTROL_CODE\r\n");
            break;
        case ERROR_INPUT_ALPHANUMERIC:
            PrintUART((uint8_t *)"ERROR_INPUT_ALPHANUMERIC\r\n");
            break;
        case ERROR_INPUT_HEX:
            PrintUART((uint8_t *)"ERROR_INPUT_HEX\r\n");
            break;
        case ERROR_UART_PRINTUART_ENDOFLINE:
            PrintUART((uint8_t *)"ERROR_UART_PRINTUART_ENDOFLINE\r\n");
            break;
        case ERROR_LINBLE_SENDFAILURE:
            PrintUART((uint8_t *)"ERROR_LINBLE_SENDFAILURE\r\n");
            break;
        case ERROR_LINBLE_NOTFIND_ENDOFLINE:
            PrintUART((uint8_t *)"ERROR_LINBLE_NOTFIND_ENDOFLINE\r\n");
            break;
        case ERROR_LINBLE_NOTFIND_CR:
            PrintUART((uint8_t *)"ERROR_LINBLE_NOTFIND_CR\r\n");
            break;
        case ERROR_MYSTRINGFUNC_NOTFIND_ENDOFLINE:
            PrintUART((uint8_t *)"ERROR_MYSTRINGFUNC_NOTFIND_ENDOFLINE\r\n");
            break;
        case ERROR_LINBLE_RECIEVEFAILURE:
            PrintUART((uint8_t *)"ERROR_LINBLE_RECIEVEFAILURE\r\n");
            break;

        default:
            PrintUART((uint8_t *)"Unregistered error\r\n");
            break;
    }
}

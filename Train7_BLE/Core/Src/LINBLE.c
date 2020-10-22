/*
 * LINBLE.c
 *
 *  Created on: Oct 19, 2020
 *      Author: fuminori.hakoishi
 */

// FA:D5:EA:28:DA:D2

#include "LINBLE.h"

#include "UART.h"
#include "mystringfunc.h"
#include "string.h"

#define BUF_STR_SIZE 64
#define LINBLE_RECEIVE_BUF 64

typedef enum {
    LINBLE_STATE_COMMAND,
    LINBLE_STATE_ADVERTISE,
    LINBLE_STATE_ONLINE,
} LINBLE_State_Type;

static int8_t LINBLEStatus = LINBLE_STATE_COMMAND;

// 受信バッファ
static uint8_t LINBLE_ReceiveData1[LINBLE_RECEIVE_BUF];
// 最後の文字のバッファ
static uint8_t LINBLE_ReceiveCharLast;

// 受信した文字数
static uint8_t LINBLE_ReceiveCount;
// 最後のカウント
static uint8_t LINBLE_ReceiveCountLast;

static UART_HandleTypeDef *this_huart;

static uint8_t recieveBuf;

int8_t LINBLE_sendCmd(uint8_t *i_cmd, uint8_t i_cmdLen, uint8_t *o_strbuf);

// 初期化

void LINBLE_Init(UART_HandleTypeDef *huart) {
    // シリアルのインスタンスを格納
    this_huart = huart;

    // LINBLE UART 受信設定
    HAL_UART_Receive_IT(this_huart, &recieveBuf, 1);

    // BLEのイニシャライズ
    // リセット後、500ms以上待つ必要がある
    HAL_Delay(500);
}

// 受信したデータを格納する
void LINBLE_SetReceiveData(void) {
    uint8_t i_data = recieveBuf;

    // 受信したデータを格納
    LINBLE_ReceiveData1[LINBLE_ReceiveCount] = i_data;

    // 前回の位置を記録
    LINBLE_ReceiveCountLast = LINBLE_ReceiveCount;

    // 入力桁数を増加
    LINBLE_ReceiveCount++;

    // 入力がバッファを超えたら、
    if (LINBLE_ReceiveCount >= LINBLE_RECEIVE_BUF) {
        // 最後に終端文字を入れる
        LINBLE_ReceiveData1[LINBLE_RECEIVE_BUF] = '\0';

        // バッファを最初からにする
        LINBLE_ReceiveCount = 0;
    }

    HAL_UART_Receive_IT(this_huart, &recieveBuf, 1);
}

uint8_t LINBLE_GetReceiveCharLast(void) {
    return recieveBuf;
}

// バッファに入っているデータを取得する
uint8_t LINBLE_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize) {
    uint8_t i = 0;

    while (i <= LINBLE_ReceiveCountLast && i < i_bufSize) {
        *o_strAddr = LINBLE_ReceiveData1[i];
        i++;
        o_strAddr++;
    }

    return i;
}

void LINBLE_Wait(void) {
    int8_t l_retMesg = 0;

    switch (LINBLEStatus) {
        case LINBLE_STATE_COMMAND:

            // コマンド状態から、アドバタイズ状態へ遷移させる
            // BTA<CR>コマンドを送信する

            l_retMesg = LINBLE_StartConnection();
            if (l_retMesg != 0) {
                PrintUART((uint8_t *)"State_runBLE error\r\n");
            } else {
                // アドバタイズ状態へ遷移
                LINBLEStatus = LINBLE_STATE_ADVERTISE;
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

// ペリフェラルのLINBLEをアドバタイズ状態へ遷移させ、セントラルに接続させる
// 接続が成功すると、0が返る
// 接続が失敗すると、-1か返る
int8_t LINBLE_StartConnection(void) {
    uint8_t l_ret[BUF_STR_SIZE];
    int8_t l_errorState = -1;

    // コマンドを送信
    // if (LINBLE_sendCmd("BTA\r", 4, &l_ret) != 0) {
    //     PrintUART("sendCmd error\r\n");
    //     return -2;
    // }

    l_errorState = HAL_UART_Transmit_IT(this_huart, "BTA\r", 4);

    HAL_UART_Receive_IT(this_huart, &recieveBuf, 1);

    if (l_errorState != 0) {
        PrintUART("error StartConnection\r\n");

        return -1;
    } else {
        return 0;
    }

    // コマンドを受信
    //	HAL_UART_Receive_IT()

    // もし、返ってきた文字列が成功（CONN<CR><LF>）の場合、
    // if (strncmp(l_ret, "CONN\r\n", 6) == 0) {
    //     return 0;
    // } else {
    //     return -1;
    // }
}

int8_t LINBLE_ShowVersion(void) {
    uint8_t l_ret[BUF_STR_SIZE];

    // コマンドを送信
    if (LINBLE_sendCmd("BTZ\r\0", 5, &l_ret) != 0) {
        PrintUART("sendCmd error\r\n");
        return -2;
    }
    PrintUART(l_ret);
    return 0;
}

int8_t LINBLE_ShowDeviceName(void) {
    uint8_t l_ret[BUF_STR_SIZE];

    // コマンドを送信
    if (LINBLE_sendCmd("BTM\r\0", 5, &l_ret) != 0) {
        PrintUART("sendCmd error\r\n");
        return -2;
    }
    PrintUART(l_ret);
    return 0;
}

/* -------------------------------------------------- */
// ローカル関数
/* -------------------------------------------------- */

int8_t LINBLE_sendCmd(uint8_t *i_cmd, uint8_t i_cmdLen, uint8_t *o_strbuf) {
    // エラーコード格納用
    int l_status;
    // 送信用バッファ
    uint8_t l_cmd[i_cmdLen + 1];
    // 受信用バッファ
    uint8_t l_recieveBuf[BUF_STR_SIZE];

    memset(l_cmd, '\0', i_cmdLen + 1);
    memset(l_recieveBuf, '\0', BUF_STR_SIZE);

    /* ----------------------- エラー処理 -------------------*/

    // CRが存在するかチェック
    if (MyString_FindCR(i_cmd, i_cmdLen) <= 0) {
        PrintERROR(ERROR_LINBLE_NOTFIND_ENDOFLINE);
        return -1;
    }

    /* -------------------------------------------------- */
    uint8_t l_loopCnt2 = 0;
    for (l_loopCnt2 = 0; l_loopCnt2 < i_cmdLen; l_loopCnt2++) {
        // 受取った文字列をコマンドへ変換
        l_cmd[l_loopCnt2] = i_cmd[l_loopCnt2];
    }

    // シリアルに送信
    PrintUART(l_cmd);
    l_status = HAL_UART_Transmit_IT(this_huart, l_cmd, strlen(l_cmd));

    // 送信が失敗したらエラー
    if (l_status != HAL_OK) {
        // エラーコード
        dprintUART("transmit error : ", l_status);
        PrintERROR(ERROR_LINBLE_SENDFAILURE);
        return -2;
    }

    // 受信準備
    // l_status = HAL_UART_Receive_IT(this_huart, &l_recieveBuf, BUF_STR_SIZE - 1);

    if (l_status != HAL_OK) {
        // エラーコード
        dprintUART("recieve error : ", l_status);
        PrintERROR(ERROR_LINBLE_RECIEVEFAILURE);
        return -2;
    }

    if (l_recieveBuf[0] == '\0') {
        PrintUART("Error null\r\n");
        return -1;
    } else {
        PrintUART(l_recieveBuf);

        // コピー
        uint8_t l_loopCnt = 0;
        while (l_loopCnt < BUF_STR_SIZE) {
            *o_strbuf = l_recieveBuf[l_loopCnt];
            l_loopCnt++;
        }
    }

    return 0;
}

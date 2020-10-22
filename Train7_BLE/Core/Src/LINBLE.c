/*
 * LINBLE.c
 *
 *  Created on: Oct 19, 2020
 *      Author: fuminori.hakoishi
 */

// FA:D5:EA:28:DA:D2

#include "LINBLE.h"

#include "State.h"
#include "UART.h"
#include "mystringfunc.h"
#include "string.h"

#define BUF_STR_SIZE 64
#define LINBLE_RECEIVE_BUF 64

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

// エンドラインフラグ
static bool LINBLE_EndLineFlg = false;

// 受信待機フラグ
static bool LINBLE_ReceiveResultMesgWaitFlg = false;

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

    if (i_data == '\n') {
        LINBLE_EndLineFlg = true;
    }

    // 受信したデータを格納
    LINBLE_ReceiveData1[LINBLE_ReceiveCount] = i_data;

    // 前回の位置を記録
    LINBLE_ReceiveCountLast = LINBLE_ReceiveCount;

    // 入力桁数を増加
    LINBLE_ReceiveCount++;

    // 入力がバッファを超えたら、
    if (LINBLE_ReceiveCount >= LINBLE_RECEIVE_BUF - 1) {
        // 最後に終端文字を入れる
        LINBLE_ReceiveData1[LINBLE_RECEIVE_BUF - 1] = '\0';

        // バッファを最初からにする
        LINBLE_ReceiveCount = 0;
    }

    HAL_UART_Receive_IT(this_huart, &recieveBuf, 1);
}

uint8_t LINBLE_GetReceiveCharLast(void) {
    return recieveBuf;
}
bool LINBLE_GetEndLineFlg(void) {
    return LINBLE_EndLineFlg;
}
void LINBLE_ClrEndLineFlg(void) {
    LINBLE_EndLineFlg = false;
}

// バッファに入っているデータを取得する
uint8_t LINBLE_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize) {
    uint8_t i = 0;

    // バッファサイズより大きい場合の例外
    if (LINBLE_ReceiveCountLast > i_bufSize) {
        PrintUART("error linble getreceivedata \r\n");
        return 0;
    }

    while (i <= LINBLE_ReceiveCountLast && i < i_bufSize) {
        *o_strAddr = LINBLE_ReceiveData1[i];
        i++;
        o_strAddr++;
    }

    return i;
}

uint8_t LINBLE_GetState(void) {
    return LINBLEStatus;
}

uint8_t LINBLE_SetState(uint8_t i_state) {
    switch (i_state) {
        case LINBLE_STATE_COMMAND:
            LINBLEStatus = i_state;
            break;
        case LINBLE_STATE_ADVERTISE:
            LINBLEStatus = i_state;
            break;
        case LINBLE_STATE_ONLINE:
            LINBLEStatus = i_state;
            break;
        default:
            PrintUART("error linble setstate\r\n");
            LINBLEStatus = 0xFF;
            break;
    }
}

void LINBLE_EnterHandler(uint8_t i_sysState) {
    uint8_t l_strBuf[64];
    uint8_t l_strLength;

    switch (i_sysState) {
        case SYS_STATE_BLE:

            switch (LINBLEStatus) {
                case LINBLE_STATE_COMMAND:

                    // コマンド状態から、アドバタイズ状態へ遷移させる
                    // BTA<CR>コマンドを送信する

                    l_strLength = UART_GetReceiveData(&l_strBuf, 64);
                    if (l_strLength == 2) {
                        switch (l_strBuf[0]) {
                            case '1':
                                PrintUART("pushed 1, Start connection.\r\n");
                                LINBLE_StartConnection();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;

                                break;
                            default:
                                break;
                        }
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

            break;

        default:
            // 何もしない
            break;
    }
}

bool LINBLE_GetReceiveResultMesgWaitFlg(void) {
    return LINBLE_ReceiveResultMesgWaitFlg;
}

void LINBLE_ClrReceiveWaitFlg(void) {
    LINBLE_ReceiveResultMesgWaitFlg = false;
}

// ペリフェラルのLINBLEをアドバタイズ状態へ遷移させ、セントラルに接続させる
// 接続が成功すると、0が返る
// 接続が失敗すると、-1か返る
int8_t LINBLE_StartConnection(void) {
    if (LINBLE_SendCommandToLINBLE("BTA\r", 4) != 0) {
        PrintUART("error StartConnection\r\n");
        return -1;
    } else {
        return 0;
    }
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

int8_t LINBLE_SendCommandToLINBLE(uint8_t *i_cmd, uint8_t i_cmdSize) {
    uint8_t l_ret[BUF_STR_SIZE];
    int8_t l_errorState = -1;

    if (LINBLE_ReceiveResultMesgWaitFlg == true) {
        PrintUART("The result message has not come back yet\r\n");
        return -1;
    }

    l_errorState = HAL_UART_Transmit_IT(this_huart, i_cmd, i_cmdSize);

    if (l_errorState != 0) {
        PrintUART("error sendcommandtolinble\r\n");

        return -1;
    } else {
        return 0;
    }
}

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

static UART_HandleTypeDef *this_huart;

static int8_t LINBLEStatus = LINBLE_STATE_COMMAND;

// 受信バッファ
static uint8_t LINBLE_ReceiveData1[LINBLE_RECEIVE_BUF];
// 最後の文字のバッファ
static uint8_t LINBLE_ReceiveCharLast;

// 受信した文字数
static uint8_t LINBLE_ReceiveCount;
// 最後のカウント
static uint8_t LINBLE_ReceiveCountLast;

static uint8_t recieveBuf;

// エンドラインフラグ
static bool LINBLE_EndLineFlg = false;

// 受信待機フラグ
static bool LINBLE_ReceiveResultMesgWaitFlg = false;

// 未読バッファカウンタ（1バイト）
static uint8_t LINBLE_ReceiveDataUnReadCount = 0;

typedef struct {
    bool bti;
    bool btc;
} LINBLE_FLG_CMD_Type;

// コマンドのフラグ監理変数
static LINBLE_FLG_CMD_Type LINBLE_cmdFlg;

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

    // リザルトメッセージの最後が<CR><LF>なことを利用して、コマンドの最後を検出
    if (i_data == '\n') {
        LINBLE_EndLineFlg = true;
    }

    // 受信したデータを格納
    LINBLE_ReceiveData1[LINBLE_ReceiveCount] = i_data;

    // 前回の位置を記録
    LINBLE_ReceiveCountLast = LINBLE_ReceiveCount;

    // 入力桁数を増加
    LINBLE_ReceiveCount++;

    // 未読バッファカウンタを増加
    LINBLE_ReceiveDataUnReadCount++;

    // 入力がバッファを超えたら、
    if (LINBLE_ReceiveCount >= LINBLE_RECEIVE_BUF - 1) {
        // 最後に終端文字を入れる
        LINBLE_ReceiveData1[LINBLE_RECEIVE_BUF - 1] = '\0';

        // バッファを最初からにする
        LINBLE_ReceiveCount = 0;
    }
}

uint8_t LINBLE_GetReceiveDataUnReadCount(void) {
    return LINBLE_ReceiveDataUnReadCount;
}

int8_t LINBLE_DecReceiveDataUnReadCount(void) {
    if (LINBLE_ReceiveDataUnReadCount > 0) {
        LINBLE_ReceiveDataUnReadCount--;
    } else {
        PrintUART("LINBLE DecReceiveDataUnReadCount() error\r\n");
        return -1;
    }
}

// mainの受信割込みで呼ばれる
void LINBLE_ReloadReceiveInterrupt(void) {
    HAL_UART_Receive_IT(this_huart, &recieveBuf, 1);
}

uint8_t LINBLE_GetReceiveCountLast(void) {
    return LINBLE_ReceiveCountLast;
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

// バッファに入っている全てのデータを取得する
uint8_t LINBLE_GetReceiveData(uint8_t *o_strAddr, uint8_t i_bufSize) {
    uint8_t i = 0;

    // バッファサイズより大きい場合の例外
    // if (LINBLE_ReceiveCountLast > i_bufSize - 1) {
    // PrintUART("error データがバッファに入らない\r\n");
    // return 0;
    // }

    while (i <= LINBLE_ReceiveCountLast && i < i_bufSize - 1) {
        *o_strAddr = LINBLE_ReceiveData1[i];
        i++;
        o_strAddr++;
        LINBLE_DecReceiveDataUnReadCount();
    }

    *o_strAddr = '\0';

#ifdef MYDEBUG_LINBLE_GETRECEIVEDATA
    PrintUART(o_strAddr);
    PrintUART("\r\n");
#endif

    return i;
}

// バッファに入っているデータを後ろからi_bufSize - 1 文字取得する
uint8_t LINBLE_GetReceiveDataLast(uint8_t *o_strAddr, uint8_t i_bufSize) {
    uint8_t i   = 0;
    uint8_t cnt = 0;

    // バッファに入っている文字列より、取得しようとしている文字列の方が長い場合、
    if (LINBLE_ReceiveCountLast + 1 < i_bufSize - 1) {
        i = 0;
    } else {
        i = (LINBLE_ReceiveCountLast + 1) - (i_bufSize - 1);
    }

    // バッファサイズより大きい場合の例外
    // if (LINBLE_ReceiveCountLast > i_bufSize - 1) {
    // PrintUART("error データがバッファに入らない\r\n");
    // return 0;
    // }

    while (i <= LINBLE_ReceiveCountLast) {
        *o_strAddr = LINBLE_ReceiveData1[i];
        i++;
        o_strAddr++;
        cnt++;
        LINBLE_DecReceiveDataUnReadCount();
    }

    *o_strAddr = '\0';

    return cnt;
}

// バッファカウントをクリア
// バッファをリードした後、バッファのカウンタを最初にしたいときに使用する
void LINBLE_BufferCountClear(void) {
    LINBLE_ReceiveCount = 0;
}

uint8_t LINBLE_GetState(void) {
    return LINBLEStatus;
}

uint8_t LINBLE_SetState(uint8_t i_state) {
    switch (i_state) {
        case LINBLE_STATE_COMMAND:
        case LINBLE_STATE_ADVERTISE:
        case LINBLE_STATE_ONLINE:
            LINBLEStatus = i_state;
            break;
        default:
            PrintUART("error linble setstate\r\n");
            LINBLEStatus = 0xFF;
            break;
    }
}

// エンターキーが押された時に実行される
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
                    if (l_strLength > 0) {
                        switch (l_strBuf[0]) {
                            case '1':
                                PrintUART("pushed 1. Try Start connection.\r\n");
                                LINBLE_SendCmdStartConnection();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;

                                break;
                            case '2':
                                // バージョンを表示する
                                LINBLE_SendCmdShowVersion();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;
                                break;
                            case '3':
                                // LINBLEのデバイス名を表示する
                                LINBLE_SendCmdShowDeviceName();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;
                                break;
                            case '4':
                                // LINBLEの現在の状態を取得する
                                LINBLE_SendCmdCheckStatus();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;
                                break;

                            default:
                                PrintUART("not commmand.\r\n");
                                break;
                        }
                    } else {
                        PrintUART("error BLE LINBLE ENTER HANDLER\r\n");
                    }

                    break;

                case LINBLE_STATE_ADVERTISE:

                    break;

                case LINBLE_STATE_ONLINE:

                    l_strLength = UART_GetReceiveData(&l_strBuf, 64);
                    if (l_strLength > 0) {
                        if (PrintLINBLE(&l_strBuf, l_strLength) == true) {
                            PrintUART("Send Done, to LINBLE.\r\n");
                            // PrintUARTn(&l_strBuf, l_strLength);
                        } else {
                            PrintUART("Not Send, to LINBLE.\r\n");
                        }
                    } else {
                        PrintUART("BLE linble online enter error\r\n");
                    }

                    break;
                default:
                    PrintUART((uint8_t *)"Error runBLE : switch default reached.\r\n");
                    break;
            }

            break;

        case SYS_STATE_BLE_CENTRAL:
            switch (LINBLEStatus) {
                case LINBLE_STATE_COMMAND:
                    l_strLength = UART_GetReceiveData(&l_strBuf, 64);

// #define MYDEBUG_LINBLE_CENTRAL_ENTER
#ifdef MYDEBUG_LINBLE_CENTRAL_ENTER
                    PrintUART("debug : UART_GetReceiveData() :");
                    PrintUARTInt(l_strLength);
                    PrintUART("\r\n");
#endif

                    if (l_strLength > 0) {
                        switch (l_strBuf[0]) {
                            case '1':
                                PrintUART("pushed 1, Start scan around.\r\n");
                                LINBLE_SendCmdScanDevice();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;
                                break;
                            case '2':
                                // バージョンを表示する
                                LINBLE_SendCmdShowVersion();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;
                                break;
                            case '3':
                                // LINBLEのデバイス名を表示する
                                LINBLE_SendCmdShowDeviceName();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;
                                break;
                            case '4':
                                // LINBLEの現在の状態を取得する
                                LINBLE_SendCmdCheckStatus();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;
                                break;
                            case '5':
                                // 検索した1番目のデバイスに接続する
                                LINBLE_SendCmdConnectPeripheral();
                                // 受信待機フラグ
                                LINBLE_ReceiveResultMesgWaitFlg = true;
                                break;

                            default:
                                PrintUART("not Command.\r\n");
                                break;
                        }
                    } else {
                        PrintUART("BLE Central command string 0 error.\r\n");
                    }
                    break;

                case LINBLE_STATE_ONLINE:
                    l_strLength = UART_GetReceiveData(&l_strBuf, 64);

                    if (l_strLength > 0) {
                        if (PrintLINBLE(&l_strBuf, l_strLength) == true) {
                            PrintUART("Send Done, to LINBLE.\r\n");
                            // PrintUARTn(&l_strBuf, l_strLength);
                        } else {
                            PrintUART("Not Send, to LINBLE.\r\n");
                        }
                    } else {
                        PrintUART("linble online enter error\r\n");
                    }

                    break;
                // SYS_STATE_BLE_CENTRAL switch
                default:
                    break;
            }

        // sys_state
        default:
            // 何もしない
            break;
    }
}

bool LINBLE_GetReceiveResultMesgWaitFlg(void) {
    return LINBLE_ReceiveResultMesgWaitFlg;
}

void LINBLE_ClrReceiveResultMesgWaitFlg(void) {
    LINBLE_ReceiveResultMesgWaitFlg = false;
}

// ペリフェラルのLINBLEをアドバタイズ状態へ遷移させ、セントラルに接続させる
// 接続が成功すると、0が返る
// 接続が失敗すると、-1か返る
int8_t LINBLE_SendCmdStartConnection(void) {
    if (LINBLE_SendCmdStrToLINBLE("BTA\r", 4) != 0) {
        PrintUART("error StartConnection\r\n");
        return -1;
    } else {
        return 0;
    }
}

int8_t LINBLE_SendCmdCheckStatus(void) {
    if (LINBLE_SendCmdStrToLINBLE("BTE\r", 4) != 0) {
        PrintUART("error CheckStatus()\r\n");
        return -1;
    } else {
        return 0;
    }
}

int8_t LINBLE_SendCmdShowVersion(void) {
    if (LINBLE_SendCmdStrToLINBLE("BTZ\r", 4) != 0) {
        PrintUART("error LINBLE_SendCmdShowVersion()\r\n");
        return -1;
    } else {
        return 0;
    }
}

int8_t LINBLE_SendCmdShowDeviceName(void) {
    if (LINBLE_SendCmdStrToLINBLE("BTM\r", 4) != 0) {
        PrintUART("error LINBLE_SendCmdShowDeviceName()\r\n");
        return -1;
    } else {
        return 0;
    }
}

// 最大8台、タイムアウト2秒
int8_t LINBLE_SendCmdScanDevice(void) {
    if (LINBLE_SendCmdStrToLINBLE("BTI82\r", 6) != 0) {
        PrintUART("error LINBLE_SendCmdScanDevice()\r\n");
        return -1;
    } else {
        LINBLE_cmdFlg.bti = true;
        return 0;
    }
}

// セントラル側がペリフェラル側に接続する
int8_t LINBLE_SendCmdConnectPeripheral(void) {
    if (LINBLE_SendCmdStrToLINBLE("BTC1\r", 5) != 0) {
        PrintUART("error LINBLE_SendCmdConnectPeripheral()\r\n");
        return -1;
    } else {
        LINBLE_cmdFlg.btc = true;
        return 0;
    }
}

int8_t LINBLE_SendCmdStrToLINBLE(uint8_t *i_cmd, uint8_t i_cmdSize) {
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

int8_t LINBLE_ReceiveDataBTI(uint8_t *i_strBuf, uint8_t i_bufSize) {
    static l_btc_ack_state = false;

    if (l_btc_ack_state == false) {
        // acknを受信、次のconnに備える
        if (Mystring_FindStrFromEnd(i_strBuf, i_bufSize, "ACKN\r\n", 6) == 1) {
            PrintUART("LINBLE_ReceiveDataBTI() read ackn\r\n");
            l_btc_ack_state = true;
        } else {
        }
    } else {
        // TERMが来るまでリードする
        if (Mystring_FindStrFromEnd(i_strBuf, i_bufSize, "TERM\r\n", 6) != 1) {
            PrintUART("Scan device : ");
            PrintUART(i_strBuf);
            PrintUART("\r\n");
        } else {
            PrintUART("LINBLE_ReceiveDataBTI() read TERN\r\n");
            l_btc_ack_state   = false;
            LINBLE_cmdFlg.bti = false;
        }
    }
}

// ペリフェラルへの接続要求の受信待機
// btcフラグが立っている時の受信処理
int8_t LINBLE_ReceiveDataBTC(uint8_t *i_strBuf, uint8_t i_bufSize) {
    static l_btc_ack_state = false;

    if (l_btc_ack_state == false) {
        // acknを受信、次のconnに備える
        if (Mystring_FindStrFromEnd(i_strBuf, i_bufSize, "ACKN\r\n", 6) == 1) {
            PrintUART("LINBLE_ReceiveDataBTC() read ackn\r\n");
            l_btc_ack_state = true;
        } else {
        }
    } else {
        // connを受信、LINBLEの状態をコマンド状態から、オンライン状態へ遷移
        if (Mystring_FindStrFromEnd(i_strBuf, i_bufSize, "CONN\r\n", 6) == 1) {
            PrintUART("ReceiveDataBTC() read conn\r\n");
            LINBLE_SetState(LINBLE_STATE_ONLINE);
            l_btc_ack_state   = false;
            LINBLE_cmdFlg.btc = false;
        } else {
        }
    }
}

// コマンド実行フラグの構造体のポインタを返す
bool LINBLE_GetCmdFlg(uint8_t i_cmd) {
    bool l_retBool = false;

    switch (i_cmd) {
        case LINBLE_FLG_CMD_BTI:
            l_retBool = LINBLE_cmdFlg.bti;
            break;
        case LINBLE_FLG_CMD_BTC:
            l_retBool = LINBLE_cmdFlg.btc;
            break;
        default:
            break;
    }

    return l_retBool;
}

int8_t PrintLINBLE(uint8_t *i_str, uint8_t i_size) {
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

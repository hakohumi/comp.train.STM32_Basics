/*
 * Dump.c
 *
 *  Created on: 2020/10/14
 *      Author: fuminori.hakoishi
 */

#include "Dump.h"

#include "LCD.h"
#include "UART.h"

void Dump_readMemLine(uint8_t *i_memStartAddr, uint8_t i_readSize, uint8_t *o_memDataArray);

/*
 * UARTで、1行ずつ流す
 * 流すのにDMAを使用する
 *
 * 1行目　　　　　　　+0 +1 +2 +3
 * 2行目  0xFFFF FFF0
 */

// バッファのサイズ
#define BUF_STR_SIZE 128

// 1行に表示するデータの数
#define BUF_LINE_SIZE 16

// 列の区切り文字
#define LINE_SEPARATE ' '
//#define LINE_SEPARATE '\t'

// アスキー 文字の区切り文字
#define LINE_SEPARATE_CHARA ','

// 指定された開始アドレスから指定した読み取るサイズ（単位：バイト）分のメモリを読み取ってシリアルに流す
// リードする範囲は(0x00000000 ~ 0xFFFFFFFF)
// 流れ
// 指定した範囲を一行ずつメモリをリードする関数を呼び出す

int8_t Dump_sendMemDumpUART(uint8_t *i_memStartAddr, uint32_t i_readSize) {
    // 文字列表示用バッファ
    uint8_t l_strBuf[BUF_STR_SIZE];
    // メモリデータをアスキー表示用文字列バッファ
    uint8_t l_strASCIIBuf[BUF_STR_SIZE];
    // 1バイトデータ書き込み用文字列バッファ
    uint8_t l_strCharaBuf[3];
    // メモリのデータ1行分の配列
    uint8_t l_memDataArray[BUF_LINE_SIZE];
    // メモリを読み取る箇所のアドレス
    uint8_t *l_memReadDataPtr;
    // 表示行頭アドレス
    uint8_t *l_memBeginLineAddr;

    // 終了アドレス
    uint8_t *l_memEndAddr;

    // 読み取る行頭アドレスのオフセット
    uint8_t l_startAddrOffset = 0;
    // 読み取る行末アドレスのオフセット
    uint8_t l_endAddrOffset = 0;

    // ロールオーバー検知用
    uint8_t *l_oldMemBeginLineAddr;

    // for文用添え字
    uint8_t i;

    // 1行目表示
    // 12文字空白後、「+0 +1 +2 +3」
    // BUF_LINE_SIZEによって、列数を変更する
    sprintf(l_strBuf, "Address\t\t");
    //	sprintf(l_strBuf, "Address\t\t%c", LINE_SEPARATE);
    for (i = 0; i < BUF_LINE_SIZE; i++) {
        if (i == 8) {
            sprintf(l_strBuf, "%s  %2d", l_strBuf, i);
        } else {
            sprintf(l_strBuf, "%s%c%2d", l_strBuf, LINE_SEPARATE, i);
        }
    }

    PrintUART(l_strBuf);
    PrintUART("\r\n");

    // 2行目以降

    // 指定されたアドレスの位置のデータを持ってくる
    /* -------------------------------------------------------------- */
    // チェック
    /* -------------------------------------------------------------- */

    // サイズのチェック
    if (i_readSize == 0) {
        PrintUART("指定されたサイズが異常です。\r\n");
        return -1;
    }

    // 終了アドレスの範囲チェック
    // 終了アドレス
    l_memEndAddr = i_memStartAddr + i_readSize - 1;

    //		if ((i_memStartAddr + i_readSize -1 ) > 0xFFFFFFFF && (i_memStartAddr + i_readSize -1 ) < 0) {
    if (i_memStartAddr > l_memEndAddr || (i_memStartAddr + i_readSize - 1) < i_memStartAddr) {
        // エラー処理
        PrintUART("終了アドレスが異常です\r\n");
        return -1;
    }
    /* -------------------------------------------------------------- */

    // 行頭のアドレス
    l_memBeginLineAddr = (uint8_t *)(((uint32_t)i_memStartAddr / (uint32_t)BUF_LINE_SIZE) * (uint32_t)BUF_LINE_SIZE);

    // 開始アドレスが行頭のアドレスではない時に
    //開始 オフセットを算出する
    if (l_memBeginLineAddr != i_memStartAddr) {
        // ズレ(1 ~ BUF_LINE_SIZE - 1)を算出
        l_startAddrOffset = i_memStartAddr - l_memBeginLineAddr;
    }

    // 指定したサイズの行数分、1行分メモリを読んで表示させるを繰り返す
    while (l_memBeginLineAddr <= l_memEndAddr) {
        // 指定された行頭アドレスを表示
        sprintf(l_strBuf, "0x%08x\t", l_memBeginLineAddr);

        // もし、最後の行になったら、リードする数を減らす
        l_endAddrOffset = (uint32_t)((l_memEndAddr - l_memBeginLineAddr) / BUF_LINE_SIZE);
        if (l_endAddrOffset == 0) {
            l_endAddrOffset = (l_memEndAddr - l_memBeginLineAddr) % BUF_LINE_SIZE;
        } else {
            l_endAddrOffset = BUF_LINE_SIZE;
        }

        // もし、行頭オフセットがある場合
        if (l_startAddrOffset > 0) {
            // 1行データを読む
            Dump_readMemLine(l_memBeginLineAddr + l_startAddrOffset,
                BUF_LINE_SIZE - l_startAddrOffset,
                &l_memDataArray[l_startAddrOffset]);
        } else {
            // 1行データを読む
            Dump_readMemLine(l_memBeginLineAddr, l_endAddrOffset, l_memDataArray);
        }

        // 初期化
        memset(l_strASCIIBuf, '\0', BUF_STR_SIZE);
        memset(l_strCharaBuf, '\0', 3);

        // アドレスから行数分のメモリ内のデータを表示
        for (i = 0; i < BUF_LINE_SIZE; i++) {
            memset(l_strCharaBuf, '\0', 3);

            // Hex
            // オフセットがある場合、その場所まで空白にする
            if (i < l_startAddrOffset) {
                memcpy(l_strCharaBuf, "..", 2);
            } else if (i > l_endAddrOffset) {
                memcpy(l_strCharaBuf, "..", 2);
            } else {
                sprintf(l_strCharaBuf, "%02x", l_memDataArray[i]);
            }

            // 8列目で、空行を2つ入れる
            if (i == 8) {
                sprintf(l_strBuf, "%s%c%c%s", l_strBuf, LINE_SEPARATE, LINE_SEPARATE, l_strCharaBuf);

            } else {
                sprintf(l_strBuf, "%s%c%s", l_strBuf, LINE_SEPARATE, l_strCharaBuf);
            }

            memset(l_strCharaBuf, '\0', 3);

            // ASCII
            // オフセットがある場合、その場所まで空白にする
            if (i < l_startAddrOffset) {
                // アスキー表示用バッファにも、空白を格納
                sprintf(l_strCharaBuf, ".");

            } else if (i > l_endAddrOffset) {
                // アスキー表示用バッファにも、空白を格納
                sprintf(l_strCharaBuf, ".");
            } else {
                // 制御文字の回避
                if (MyString_CheckCharCtrlCode(l_memDataArray[i]) == false) {
                    // 制御文字の場合
                    sprintf(l_strCharaBuf, ".");
                } else {
                    // 英数字の場合
                    sprintf(l_strCharaBuf, "%1c", l_memDataArray[i]);
                }
            }

            sprintf(l_strASCIIBuf, "%s%s", l_strASCIIBuf, l_strCharaBuf);
        }

        // オフセットの初期化
        l_startAddrOffset = 0;

        // 1行のHexデータをシリアルに送信
        PrintUART(l_strBuf);

        // セパレート挿入
        PrintUART(" ");
        PrintUART(l_strASCIIBuf);

        // 改行文字の挿入
        PrintUART("\r\n");
        //		sprintf(l_strBuf, "%s\r\n", l_strBuf);

        // 1行進む
        l_oldMemBeginLineAddr = l_memBeginLineAddr;
        l_memBeginLineAddr += BUF_LINE_SIZE;

        // 変数の上限を超えてロールオーバーしているかチェック
        // 方法：前回の値より下がっていないか
        if (l_memBeginLineAddr < l_oldMemBeginLineAddr) {
            break;
        }
    }
    return 0;
}

// 指定された開始アドレスから1行分のメモリを読み取り、指定した配列へ格納する
// 入力：読み取り開始アドレス（uint32_t i_memStartAddr)、1行のサイズ(uint8_t i_readSize)
// 出力：読み取ったメモリの中身(uint8_t *oi_mem4BbyteData)(要素i_readSizeの配列)
void Dump_readMemLine(uint8_t *i_memStartAddr, uint8_t i_readSize, uint8_t *o_memDataArray) {
    uint8_t *l_pointer = i_memStartAddr;
    uint8_t i;

    for (i = 0; i < i_readSize; i++, l_pointer++, o_memDataArray++) {
        *o_memDataArray = *l_pointer;
    }
}

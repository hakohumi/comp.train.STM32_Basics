/*
 * mystringfunc.c
 *
 *  Created on: Oct 19, 2020
 *      Author: fuminori.hakoishi
 */

#include "mystringfunc.h"

#include <string.h>

#include "UART.h"

#define ENDOFLINE '\0'
#define CARRIAGERETURN '\r'
#define LINEFIELD '\n'

// プロトタイプ宣言
int8_t MyString_findChara(uint8_t *i_str, uint8_t i_bufsize, uint8_t i_chara);

int8_t MyString_FindEOL(uint8_t *i_str, uint8_t i_bufsize) {
    return MyString_findChara(i_str, i_bufsize, ENDOFLINE);
}

// <CR>チェック
int8_t MyString_FindCR(uint8_t *i_str, uint8_t i_bufsize) {
    return MyString_findChara(i_str, i_bufsize, CARRIAGERETURN);
}
// <LF>チェック
int8_t MyString_FindLF(uint8_t *i_str, uint8_t i_bufsize) {
    return MyString_findChara(i_str, i_bufsize, LINEFIELD);
}

// 文字列の最後から、文字列を見つける
// 出力はint8_t
// 入力は 検索する文字列、検索する文字列のサイズ、検索したい文字列、検索したい文字列のサイズ
int8_t Mystring_FindStrFromEnd(uint8_t *i_str, uint8_t i_strSize, uint8_t *i_searchStr, uint8_t i_searchSize) {
    uint8_t *l_cmdPnt;

    // 検索したい文字列の先頭の文字の位置をだす
    l_cmdPnt = (uint8_t*)strrchr((const char *)i_str, (int)i_searchStr[0]);

    // 未検出
    if (l_cmdPnt == NULL) {
        return -1;
    }

    // エラーになりそうなところ
    // 見つけたところから、検索したい文字列のサイズが、検索する文字列の最後の位置よりはみ出ること
    if (i_strSize - (l_cmdPnt - i_str) < i_searchSize) {
        return -1;
    }

    // 見つかったら1、見つからなかったら0
    if (strncmp((const char *)l_cmdPnt,(const char *) i_searchStr, i_searchSize) == 0) {
        return 1;
    } else {
        return 0;
    }
}

bool MyString_CheckCharCtrlCode(uint8_t i_char) {
    // 制御コードチェック
    if (i_char < 32 || 126 < i_char) {
        // エラー
        return false;
    }
    return true;
}

// とりあえず、関数化
int8_t MyString_Atoi(uint32_t *o_value, uint8_t *InputBufArray, uint8_t i_strlen) {
    uint32_t l_UART_ArgumentArray = 0;

    uint8_t i;

    // 入力された文字列のチェック
    for (i = 0; InputBufArray[i] != '\0'; i++) {
        // 制御コードチェック
        if (MyString_CheckCharCtrlCode(InputBufArray[i]) == false) {
            // エラー
            // エラーコード
            PrintERROR(ERROR_INPUT_CONTROL_CODE);
            // goto INPUT_ERROR;
            return -1;
        }

        // 英数字チェック
        if (!(('0' <= InputBufArray[i] && InputBufArray[i] <= '9') || ('A' <= InputBufArray[i] && InputBufArray[i] <= 'Z') || ('a' <= InputBufArray[i] && InputBufArray[i] <= 'z'))) {
            // エラー
            // エラーコード
            PrintERROR(ERROR_INPUT_ALPHANUMERIC);
            // goto INPUT_ERROR;
            return -1;
        }

        // 文字列を数値にした値
        uint8_t InputCharToInt = 0;

        // 16進数チェック
        if ('a' <= InputBufArray[i] && InputBufArray[i] <= 'f') {
            InputCharToInt = InputBufArray[i] - 'a' + 10;
        } else if ('A' <= InputBufArray[i] && InputBufArray[i] <= 'F') {
            InputCharToInt = InputBufArray[i] - 'A' + 10;
        } else if ('0' <= InputBufArray[i] && InputBufArray[i] <= '9') {
            InputCharToInt = InputBufArray[i] - '0';
        } else {
            PrintERROR(ERROR_INPUT_HEX);
            // goto INPUT_ERROR;
            return -1;
        }

        // 数値連続して読み込み、変数に1桁ずらしながら格納する
        l_UART_ArgumentArray =
            l_UART_ArgumentArray << 4;
        l_UART_ArgumentArray += InputCharToInt;
    }

    *o_value = l_UART_ArgumentArray;

    return 0;
}

/* -------------------------------------------------- */
// ローカル関数
/* -------------------------------------------------- */

int8_t MyString_findChara(uint8_t *i_str, uint8_t i_bufsize, uint8_t i_chara) {
    uint8_t *l_ptr      = i_str;
    uint8_t l_strlength = 0;

    // 指定した文字を見つけるまで かつ バッファ分まで
    for (l_strlength = 0; *l_ptr != i_chara && l_strlength < i_bufsize;
         l_strlength++, l_ptr++) {
    }

    // もし、終端文字が見つからなかった場合
    if (l_strlength == i_bufsize) {
        return -1;
    }

    return l_strlength;
}

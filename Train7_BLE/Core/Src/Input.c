/*
 * Input.c
 *
 *  Created on: 2020/10/20
 *      Author: fuminori.hakoishi
 */

#include "Input.h"

#include "stdbool.h"

// スイッチの押された状態
bool SWState    = SW_OFF;
bool SWStateOld = SW_OFF;

// スイッチが押された時に立つフラグ
// 外部割込みでセット、メインでクリア
bool PushedSWFlg = SW_OFF;

void Input_UpdateState(void) {
    // スイッチの過去状態を更新
    SWStateOld = SWState;

    // スイッチオン
    SWState     = PushedSWFlg;
    PushedSWFlg = SW_OFF;
}

bool Input_IsSWrise(void) {
    if (SWStateOld == SW_OFF && SWState == SW_ON) {
        return true;
    } else {
        return false;
    }
}

void Input_SetPushedSWFlg(void) {
    if (PushedSWFlg == SW_OFF) {
        // スイッチフラグ セット
        PushedSWFlg = SW_ON;
    }
}
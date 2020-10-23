/*
 * mystringfunc.h
 *
 *  Created on: Oct 19, 2020
 *      Author: fuminori.hakoishi
 */

#ifndef INC_MYSTRINGFUNC_H_
#define INC_MYSTRINGFUNC_H_

#include "main.h"
#include "stdbool.h"

// 終端文字の検索
int8_t MyString_FindEOL(uint8_t *i_str, uint8_t i_bufsize);

// 終端から文字列の検索
int8_t Mystring_FindStrFromEnd(uint8_t *i_str, uint8_t i_strSize, uint8_t *i_searchStr, uint8_t i_searchSize);

// <CR>チェック
int8_t MyString_FindCR(uint8_t *i_str, uint8_t i_bufsize);

bool MyString_CheckCharCtrlCode(uint8_t i_char);

// Atoi
int8_t MyString_Atoi(uint32_t *o_value, uint8_t *InputBufArray, uint8_t i_strlen);

#endif /* INC_MYSTRINGFUNC_H_ */

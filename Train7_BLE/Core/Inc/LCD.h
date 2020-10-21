/*
 * LCD.h
 *
 *  Created on: Oct 6, 2020
 *      Author: fuminori.hakoishi
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include <stdbool.h>

#include "main.h"

/*  LCD */

// LCDの初期化
void LCD_Init(I2C_HandleTypeDef *i_hi2c);

// print characters on LCD
void LCD_Puts(const char *p);

// 行指定
void LCD_SetPosLine(bool i_row);

void LCD_ClearBuffer(void);

// LCDバッファに書き込む
// ちゃんと文字数を指定しないと、範囲外のデータも表示するので注意
void LCD_WriteToBuffer(uint8_t i_WriteStartPos, uint8_t *i_str,
		uint8_t i_strLen);

void LCD_WriteToBufferInt(uint8_t i_WriteStartPos, uint16_t i_score,
		uint8_t i_Len);

// LCDバッファをLCDに書き込む
void LCD_BufferToLCD(void);

extern uint8_t *STR_2CHAR_BLANK;

#endif /* INC_LCD_H_ */

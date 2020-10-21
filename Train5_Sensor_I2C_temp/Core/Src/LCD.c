/*
 * LCD.c
 *
 *  Created on: Oct 6, 2020
 *      Author: fuminori.hakoishi
 */

#include "LCD.h"
//#include <stdint.h>
#include "main.h"

/* -------------------------------------------------- */
// プライベートdefine
/* -------------------------------------------------- */

// 10進数の最大数
#define DECIMAL_MAX 9

// ファンクションセットをするコマンド
#define CMD_FUNCTION_SET_IS_OFF 0x38
#define CMD_FUNCTION_SET_IS_ON 0x39

// ClearDisplay コマンドのデータ部
#define CMD_LCD_CLR_DISPLAY 0x01
// Display ON コマンドのデータ部
#define CMD_LCD_DISPLAY_ON 0x0C

// Co = 0, RS = 0, Control byte = 0;
#define CONTROLE_BYTE 0x00
// RSビットが立っているとき
#define WR_CONTROLE_BYTE 0x40

// LCDのスレーブアドレス
#define LCD_ADDR 0x3E

/* -------------------------------------------------- */
// プライベートdefine 文字列
/* -------------------------------------------------- */

// LCDに書き込む文字列保存用バッファ 16文字
#define LCD_BUFF_SIZE_MAX 16
#define LCD_LINE_LEN 8

/* -------------------------------------------------- */
// SetPos用のdefine
/* -------------------------------------------------- */

// LCDのSETPOSをするために立てるビット
#define LCD_SET_POS_DB7 0x80
// LCDの行頭のアドレス
#define LINE_FIRST_ADDR 0x00
#define LINE_SECOND_ADDR 0x40
/* -------------------------------------------------- */

/* -------------------------------------------------- */
// プライベート関数
/* -------------------------------------------------- */

// 数値一文字をchar型へ変換
uint8_t itochar(uint8_t value);

// インストラクションの送信
static int LCD_WriteInstraction(uint8_t data);
// データの送信
static int LCD_WriteData(uint8_t data);

// データをまとめて送信する
// i_data 入力 データの配列
// i_len 入力 データの件数
static int LCD_WriteNData(uint8_t *i_data, uint8_t i_len);
/* -------------------------------------------------- */

/* -------------------------------------------------- */
// プライベート変数
/* -------------------------------------------------- */

I2C_HandleTypeDef *hi2c;

static uint8_t LCDBuffer[LCD_BUFF_SIZE_MAX];

// LCD更新フラグ
// バッファに書き込んだ時のみONになる
static bool updateLCDFlg = OFF;

uint8_t STR_CHAR_BLANK = ' ';
uint8_t *STR_LINE_BLANK = "        ";

/* -------------------------------------------------- */

// LCD更新フラグをONにする
#define setUpdateLCDFlg()  \
    do {                   \
        updateLCDFlg = ON; \
    } while (0)

// LCDの初期化
void LCD_Init(I2C_HandleTypeDef *i_hi2c) {
	hi2c = i_hi2c;
	LCD_WriteInstraction(CMD_FUNCTION_SET_IS_OFF);  // Function set
	LCD_WriteInstraction(CMD_FUNCTION_SET_IS_ON);   // Function set
	LCD_WriteInstraction(0x14);                     // Internal OSC frequency
	LCD_WriteInstraction(0x70);                     // Contrast set
	LCD_WriteInstraction(0x56);                     // Power/ICON/Contrast set
	LCD_WriteInstraction(0x6C);                     // Follower control
	HAL_Delay(200);
	LCD_WriteInstraction(CMD_FUNCTION_SET_IS_OFF);  // Function set
	LCD_WriteInstraction(CMD_LCD_DISPLAY_ON);       // Display ON/OFF control
	LCD_WriteInstraction(CMD_LCD_CLR_DISPLAY);      // Clear Display
	HAL_Delay(1);
}

//print characters on LCD
void LCD_Puts(const char *p) {
	for (; *p; p++) {
		LCD_WriteData(*p);
	}
}

// 行指定
void LCD_SetPosLine(bool i_row) {
	if (i_row) {
		// true 2行目
		LCD_WriteInstraction(LCD_SET_POS_DB7 | LINE_SECOND_ADDR);
	} else {
		// false 1行目
		LCD_WriteInstraction(LCD_SET_POS_DB7 | LINE_FIRST_ADDR);
	}
}

// 　好きな位置を選択して、書き換える
void LCD_WriteToBuffer(uint8_t i_WriteStartPos, uint8_t *i_str,
		uint8_t i_strLen) {
	uint8_t i, c;

	// LCD更新フラグをON
	setUpdateLCDFlg();

	// もし、指定された文字数がLCDの残り桁数超えていたら、
	if ((16 - i_WriteStartPos) < i_strLen) {
		// エラー
		// 何もしない
	} else {
		// LCDBufferの先頭から、引数に指定された文字列をi_strLen文字コピーする
		for (i = i_WriteStartPos, c = 0; c < i_strLen; i++, c++) {
			LCDBuffer[i] = i_str[c];
		}
	}
}

// 引数 書き込み開始位置(0 ~ 15), 数値, 桁数
void LCD_WriteToBufferInt(uint8_t i_WriteStartPos, uint16_t i_val,
		uint8_t i_Len) {
	int8_t i, c;

	if ((LCD_BUFF_SIZE_MAX - (i_WriteStartPos - 1)) > i_Len) {
		// LCD更新フラグをONにする
		setUpdateLCDFlg();

		for (i = i_WriteStartPos, c = (i_Len - 1); c >= 0; c--) {
			LCDBuffer[i + c] = itochar((uint8_t) (i_val % 10));
			// 桁をずらす
			i_val /= 10;
		}
	}
}

// BufferをLCDへ書き込む
void LCD_BufferToLCD(void) {
	// バッファに変更があった時のみ更新する
	if (updateLCDFlg == ON) {
		updateLCDFlg = OFF;
		LCD_SetPosLine(false);
		LCD_WriteNData(LCDBuffer, 8);
		LCD_SetPosLine(true);
		LCD_WriteNData(&LCDBuffer[8], 8);
	}
}

/* -------------------------------------------------- */
// プライベート関数
/* -------------------------------------------------- */

// 数値一文字をchar型へ変換
uint8_t itochar(uint8_t value) {
	if (value > DECIMAL_MAX) {
		return STR_CHAR_BLANK;
	}
	return "0123456789"[value];
}

// インストラクションの送信
static int LCD_WriteInstraction(uint8_t data) {
	uint8_t buf[] = { CONTROLE_BYTE, data };
	int status = HAL_I2C_Master_Transmit(hi2c, LCD_ADDR << 1, buf, 2, 100);
	HAL_Delay(1);
	return status == HAL_OK;
}

// データの送信
static int LCD_WriteData(uint8_t data) {
	uint8_t buf[] = { WR_CONTROLE_BYTE, data };
	int status = HAL_I2C_Master_Transmit(hi2c, LCD_ADDR << 1, buf, 2, 100);
	HAL_Delay(1);
	return status == HAL_OK;
}

// データをまとめて送信する
// i_data 入力 データの配列
// i_len 入力 データの件数
static int LCD_WriteNData(uint8_t *i_data, uint8_t i_len) {
	uint8_t l_buf[i_len + 1];
	uint8_t i, c;
	int status;

	l_buf[0] = WR_CONTROLE_BYTE;

	for (c = 0; c < i_len; c++) {
		l_buf[c + 1] = i_data[c];
	}

	// 書き込み
	status = HAL_I2C_Master_Transmit(hi2c, LCD_ADDR << 1, l_buf, i_len + 1,
			100);
	HAL_Delay(1);
	return status == HAL_OK;
}

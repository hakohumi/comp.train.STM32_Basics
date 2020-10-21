#include "main.h"
#include <stdbool.h>

void LED_Blink(uint16_t i_count) {
	static bool l_LEDFlg = 0;
	static uint16_t l_count = 0;
	static bool flg = 0;

	if (flg == false) {
		// 初めて実行された時だけ、カウントを初期化する
		if (l_count == 0) {
			l_count = i_count;
			flg = true;
		}
	}

	if (flg == true) {
		if (l_count == 0) {

			if (l_LEDFlg == 1) {
//				HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
			} else {
//				HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
			}

			l_LEDFlg ^= 1;

			flg = false;

		} else {
			l_count--;
		}

	}
}

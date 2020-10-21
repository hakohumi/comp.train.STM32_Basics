/*
 * UART.c
 *
 *  Created on: Sep 29, 2020
 *      Author: fuminori.hakoishi
 */
#include "UART.h"
#include "main.h"
#include <stdbool.h>
#include <string.h>

#define DATANUM 128

/*
 * The STM32 makes receiving chars into a large circular buffer simple
 * and requires no CPU time. The UART receiver DMA must be setup as CIRCULAR.
 */
#define CIRC_BUF_SZ       64  /* must be power of two */
static uint8_t rx_dma_circ_buf[CIRC_BUF_SZ];
static UART_HandleTypeDef *huart_cobs;
static uint32_t rd_ptr;

#define DMA_WRITE_PTR ( (CIRC_BUF_SZ - huart_cobs->hdmarx->Instance->CNDTR) & (CIRC_BUF_SZ - 1) )

// private function
int readDataNum(void);

// -----------------------------------------

void msgrx_init(UART_HandleTypeDef *huart) {
	huart_cobs = huart;
	HAL_UART_Receive_DMA(huart_cobs, rx_dma_circ_buf, CIRC_BUF_SZ);
	rd_ptr = 0;
}

// 未読データ数確認
int readDataNum(void) {
	// 受信済みデータ数確認
	int index = huart_cobs->hdmarx->Instance->CNDTR; // index取得

	index = CIRC_BUF_SZ - index; // 受信データの先頭位置

	int remainData = index - rd_ptr; //読み込んでいないデータの数
	if (remainData < 0)
		remainData += CIRC_BUF_SZ;

	return remainData;

}

static bool msgrx_circ_buf_is_empty(void) {
	if (rd_ptr == DMA_WRITE_PTR) {
		return true;
	}
	return false;
}

static uint8_t msgrx_circ_buf_get(void) {
	uint8_t c = 0;
	if (rd_ptr != DMA_WRITE_PTR) {
		c = rx_dma_circ_buf[rd_ptr++];
		rd_ptr &= (CIRC_BUF_SZ - 1);
	}
	return c;
}

void viewUART(void) {
	uint8_t i = 0;

	uint8_t l_buffer[64];

//		HAL_UART_Receive(&huart2, buffer, 5, 1000);
//		HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 0xffff);

	while (msgrx_circ_buf_is_empty() == false) {
		uint8_t l_input = 0;

		// バッファに入っている件数を表示
		sprintf(l_buffer, "buf size : %d\r\n", readDataNum());
		HAL_UART_Transmit(huart_cobs, l_buffer, strlen(l_buffer), 0xffff);

		// バッファが空になるまでリードする
		for (i = 0; msgrx_circ_buf_is_empty() == false; i++) {
			l_input = msgrx_circ_buf_get();

			// 入力されたキーがエンターかどうかを判断する
			if (l_input == '\r') {
				sprintf(l_buffer, "CR\r");

			} else if (l_input == '\n') {
				sprintf(l_buffer, "LF\n");
			} else {
//				sprintf(l_buffer, "%d\r\n", readDataNum());
//				sprintf(l_buffer, "%s%c\r\n\0", l_buffer, l_input);
				HAL_UART_Transmit(huart_cobs, l_input, 1, 0xffff);

			}
		}

//		HAL_UART_Transmit(huart_cobs, l_buffer, strlen(l_buffer), 0xffff);
		HAL_UART_Transmit(huart_cobs, "\r\n", 2, 0xffff);

	}
}

void printUART(uint8_t *i_str, uint16_t i_var) {
	uint8_t l_buffer[64];
	uint8_t l_buffer2[64];

	sprintf(l_buffer, "%s : %d", i_str, i_var);
	sprintf(l_buffer2, "%s buf size : %d\r\n", l_buffer, strlen(l_buffer));

	HAL_UART_Transmit(huart_cobs, l_buffer2, strlen(l_buffer2), 0xffff);

}

/*

 // 未読データ数確認
 int readDataNum(void) {
 // 受信済みデータ数確認
 int index = huart2.hdmarx->Instance->CNDTR; // index取得

 index = DATANUM - index; // 受信データの先頭位置

 int remainData = index - indexRead; //読み込んでいないデータの数
 if (remainData < 0)
 remainData += DATANUM;
 return remainData;

 }


 uint8_t readSerial(void) {
 uint8_t readData = 0;

 int index = huart2.hdmarx->Instance->CNDTR; //index取得

 index = DATANUM - index;

 int remainData = index - indexRead; // 読み込んでいないデータの数
 if (remainData < 0)
 remainData += DATANUM;

 if (remainData > 0) {
 readData = serialData[indexRead];
 indexRead++;
 if (indexRead == DATANUM)
 indexRead = 0;
 }
 return readData;

 }

 */

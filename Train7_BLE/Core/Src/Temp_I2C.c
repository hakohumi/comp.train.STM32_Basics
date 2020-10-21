/*
 * Temp_I2C.c
 *
 *  Created on: Oct 6, 2020
 *      Author: fuminori.hakoishi
 */

#include "Temp_I2C.h"

#include "main.h"

#define TEMP_I2C_ADDR 0x48

I2C_HandleTypeDef *hi2c;

static int TempI2C_Write1Byte(uint8_t i_addr, uint8_t i_data);

// 温度センサの初期化
void TempI2C_Init(I2C_HandleTypeDef *i_hi2c) {
    hi2c = i_hi2c;
    // 0x03 の7bit目に1を書き込んで、16bit精度にしたかった
}

int TempI2C_GetTemp(void) {
    int o_temp;
    uint16_t o_temp2;

    // リードするアドレスを指定(0x00, MSB)
    int status = HAL_I2C_Master_Transmit(hi2c, TEMP_I2C_ADDR << 1, 0x00, 1, 100);

    uint8_t buf[2];

    HAL_I2C_Master_Receive(hi2c, TEMP_I2C_ADDR << 1, buf, 2, 100);

    o_temp2 = (buf[0] << 8);
    o_temp2 |= buf[1];
    o_temp = (int)o_temp2 >> 3;

    return o_temp / 16;
}

// Address pointer register byte に data byteを書き込む
static int TempI2C_Write1Byte(uint8_t i_addr, uint8_t i_data) {
    uint8_t buf[] = {i_addr, i_data};
    int status    = HAL_I2C_Master_Transmit(hi2c, TEMP_I2C_ADDR << 1, buf, 2, 100);
    HAL_Delay(1);
    return status == HAL_OK;
}

// read data from configuration register
static int TempI2C_Read1Byte() {
    uint8_t buf[2];

    HAL_I2C_Master_Receive(hi2c, TEMP_I2C_ADDR << 1, buf, 2, 100);
}

//正の温度 = ADCコード（dec） / 16 負の温度 = （ADCコード（dec）− 8192） / 16 ここで、ADCコードは符号ビットを含むデータバイトの最初の13MSBを使用します。 負の温度 = （ADCコード（dec）– 4096） / 16 ここで、ビット15（符号ビット）はADCコードから削除されます。

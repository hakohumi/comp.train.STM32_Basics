/*
 * Dump.h
 *
 *  Created on: 2020/10/14
 *      Author: fuminori.hakoishi
 */

#ifndef INC_DUMP_H_
#define INC_DUMP_H_

#include "main.h"

int8_t Dump_sendMemDumpUART(uint8_t *i_memStartAddr, uint32_t i_readSize);

#endif /* INC_DUMP_H_ */

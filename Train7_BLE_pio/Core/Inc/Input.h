/*
 * Input.h
 *
 *  Created on: 2020/10/20
 *      Author: fuminori.hakoishi
 */

#ifndef INC_INPUT_H_
#define INC_INPUT_H_

#include "main.h"
#include "stdbool.h"

#define SW_ON 0
#define SW_OFF 1

void Input_UpdateState(void);
void Input_SetPushedSWFlg(void);
bool Input_IsSWrise(void);

#endif /* INC_INPUT_H_ */

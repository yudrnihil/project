/*
 * DAC.h
 *
 *  Created on: 2016Äê4ÔÂ9ÈÕ
 *      Author: eric
 */

#ifndef DAC_H_
#define DAC_H_
#include "stm32f4xx.h"
#include "stm32f4xx_dac.h"

uint16_t Send2DAC(uint16_t d_value);
void DAC_Init();

#endif /* DAC_H_ */

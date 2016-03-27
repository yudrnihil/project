/*
 * CKey.h
 *
 *  Created on: 2016¦~3¤ë26¤é
 *      Author: YU Chendi
 */

#ifndef CKEY_H_
#define CKEY_H_

#include "stm32f4xx_conf.h"
#include "stdlib.h"
#include "delay.h"

void CKey_init();

class CKey {
	private:
		const uint16_t ADCChannel;
		const uint16_t GPIOPin;
		const uint16_t MUXChannel;
		const uint16_t frequency;
		uint16_t vref;
		uint16_t threshold;
	public:
		CKey(uint8_t id);
		void setMUX();
		void calibrate();
		uint16_t getValue();
		bool isPressed();
		void setThreshold(uint16_t threshold);
};

#endif /* CKEY_H_ */

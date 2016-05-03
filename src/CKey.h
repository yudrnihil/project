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

/**
 * Initializes ADC3 and PF0,1,2
 */
void CKey_init();

/**
 * A class for touch keys. Each key is an object of class CKey. There are
 * 24 keys in total.
 * See my schematic for pin assignment.
 */
class CKey {
	private:
		const uint16_t ADCChannel; //which channel of ADC3 is used
		const uint16_t GPIOPin;	   //which pin is used
		const uint16_t MUXChannel; //which channel of CD4051 MUX is used
		const uint16_t frequency;  //frequency of the note
		uint16_t vref;             //average voltage when the key is not pressed
		uint16_t threshold;        //threshold value to determine whether the key is pressed
		uint16_t disabled;
		uint8_t prev;
	public:
		/**
		 * constructor for CKey. Each key has an id 0~23. The constructor
		 * sets ADCChannel, GPIOPin, MUXChannel and frequency values, calibrates
		 * the key and gives a default threshold.
		 */
		CKey(uint8_t id);

		/**
		 * Sets the control pins of MUX to the desired channel
		 */
		void setMUX();

		/**
		 * Reads ADC values for the key 10 times when not pressed. Use the average
		 * as vref
		 */
		void calibrate();

		/**
		 * Reads ADC value after a discharge-charge.
		 * Must make sure the MUX is set to the correct channel before calling getValue()
		 */
		uint16_t getValue();

		/**
		 * Determines whether the key is pressed. It is pressed if
		 * getValue() < vref - threshold
		 */
//		bool isPressed();
		uint8_t isPressed();

		/**
		 * sets a new threshold
		 */
		void setThreshold(uint16_t threshold);
};

#endif /* CKEY_H_ */

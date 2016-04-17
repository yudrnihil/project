/*
 * CKey.cpp
 *
 *  Created on: 2016¦~3¤ë26¤é
 *      Author: YU Chendi
 */

#include "CKey.h"
#include "lcd.h"

static GPIO_InitTypeDef GPIO_InitStructure;
static ADC_InitTypeDef ADC_InitStructure;
static ADC_CommonInitTypeDef ADC_CommonInitStructure;

uint16_t CKey_ADCChannel[24] = {
		ADC_Channel_9, ADC_Channel_9, ADC_Channel_9, ADC_Channel_9, ADC_Channel_9, ADC_Channel_9,
		ADC_Channel_9, ADC_Channel_9, ADC_Channel_14, ADC_Channel_14, ADC_Channel_14, ADC_Channel_14,
		ADC_Channel_14, ADC_Channel_14, ADC_Channel_14, ADC_Channel_14, ADC_Channel_15, ADC_Channel_15,
		ADC_Channel_15, ADC_Channel_15, ADC_Channel_15, ADC_Channel_15, ADC_Channel_15, ADC_Channel_15};
uint16_t CKey_GPIOPin[24] = {
		GPIO_Pin_3, GPIO_Pin_3, GPIO_Pin_3, GPIO_Pin_3, GPIO_Pin_3, GPIO_Pin_3, GPIO_Pin_3, GPIO_Pin_3,
		GPIO_Pin_4, GPIO_Pin_4, GPIO_Pin_4, GPIO_Pin_4, GPIO_Pin_4, GPIO_Pin_4, GPIO_Pin_4, GPIO_Pin_4,
		GPIO_Pin_5, GPIO_Pin_5, GPIO_Pin_5, GPIO_Pin_5, GPIO_Pin_5, GPIO_Pin_5, GPIO_Pin_5, GPIO_Pin_5,};
uint16_t CKey_MUXChannel[24] = {3, 0, 1, 2, 5, 7, 6, 4, 3, 0, 1, 2, 5, 7, 6, 4, 3, 0, 1, 2, 5, 7, 6, 4};
uint16_t CKey_frequency[24] = {988, 932, 880, 831, 784, 740, 698, 659, 622, 587, 554, 523, 494, 466, 440, 415,
		392, 370, 349, 330, 311, 294, 277, 262};
enum material{Cu, Al};
uint8_t type[24] = {Al, Cu, Al, Cu, Al, Cu, Al, Al, Cu, Al, Cu, Al, Al, Cu, Al, Cu, Al, Cu, Al, Al, Cu, Al, Cu, Al};

void CKey_init(){
	//Initialize ADC3
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_Init(ADC3, &ADC_InitStructure);
	ADC_Cmd(ADC3, ENABLE);
	//Initialize PF0,1,2 for multiplexer
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
}


CKey::CKey(uint8_t id):
		ADCChannel(CKey_ADCChannel[id]), GPIOPin(CKey_GPIOPin[id]), MUXChannel(CKey_MUXChannel[id]),
		frequency(CKey_frequency[id]) {
	if(type[id] == Al ){
		threshold = 200;
	}
	else{
		threshold = 800;
	}
	setMUX();
	calibrate();
	disabled = 0;
}

void CKey::setMUX(){
	uint16_t dummy = GPIOF->ODR;
	GPIOF->ODR = (dummy & 0xfff8) | MUXChannel;
}

void CKey::calibrate(){
	uint8_t i;
	uint16_t sum = 0;
	for(i = 0; i < 10; i++){
		GPIO_InitStructure.GPIO_Pin = GPIOPin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOF, &GPIO_InitStructure);
		GPIO_ResetBits(GPIOF, GPIOPin);
		delay_us(20);
		GPIO_InitStructure.GPIO_Pin = GPIOPin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOF, &GPIO_InitStructure);
		delay_us(80);
		ADC_RegularChannelConfig(ADC3, ADCChannel, 1, ADC_SampleTime_480Cycles);
		ADC_SoftwareStartConv(ADC3);
		while(!ADC_GetFlagStatus(ADC3, ADC_FLAG_EOC));
		sum += ADC_GetConversionValue(ADC3);
	}
	vref = sum / 10;
}

uint16_t CKey::getValue() {
	GPIO_InitStructure.GPIO_Pin = GPIOPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOF, GPIOPin);
	delay_us(20);
	GPIO_InitStructure.GPIO_Pin = GPIOPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	delay_us(80);
	ADC_RegularChannelConfig(ADC3, ADCChannel, 1, ADC_SampleTime_480Cycles);
	ADC_SoftwareStartConv(ADC3);
	while(!ADC_GetFlagStatus(ADC3, ADC_FLAG_EOC));
	uint16_t result = ADC_GetConversionValue(ADC3);
	LCD_ShowNum(30,80,result,4,16);
	return result;
}

void CKey::setThreshold(uint16_t threshold) {
	this->threshold = threshold;
}

bool CKey::isPressed(){
	if (disabled){
		disabled--;
		return true;
	}
	if (getValue() < vref - threshold){
		disabled = 10;
		return true;
	}
	return false;
	//return getValue() < vref - threshold;
}



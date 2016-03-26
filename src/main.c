//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/Trace.h"
#include "stm32f4xx.h"
#include "delay.h"
#include "lcd.h"

// ----------------------------------------------------------------------------
//
// Standalone STM32F4 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

GPIO_InitTypeDef GPIO_InitStructure;
ADC_InitTypeDef ADC_InitStructure;
ADC_CommonInitTypeDef ADC_CommonInitStructure;
uint16_t CPAD_threshold;
uint16_t CPAD_ADCPin;
uint16_t CPAD_ADCChannel;
uint16_t CPAD_MUXChannel;

/**
 * initialize capacitive touch pad. use ADC3.
 */
void CPADCalibrate();
void CPADInit(){
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
	//ADC_RegularChannelConfig(ADC3, ADC_Channel_9, 1, ADC_SampleTime_480Cycles);
	//MUX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	//Initial value
//	GPIO_WriteBit(GPIOF, GPIO_Pin_0, 0);
//	GPIO_WriteBit(GPIOF, GPIO_Pin_1, 0);
//	GPIO_WriteBit(GPIOF, GPIO_Pin_2, 0);
	uint16_t dummy = GPIOF->ODR;
	GPIOF->ODR = (dummy & 0xfff8) | 3;
	CPAD_ADCPin = GPIO_Pin_3;
	CPAD_ADCChannel = ADC_Channel_9;
	CPADCalibrate();
	LCD_ShowNum(30, 120, 0, 2, 16);
}

/**
 * read the touch pad adc value 10 times. use average as threshold
 */
void CPADCalibrate(){
	uint8_t i;
	uint16_t sum = 0;
	for(i = 0; i < 10; i++){
		GPIO_InitStructure.GPIO_Pin = CPAD_ADCPin;
	    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
   	    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOF, &GPIO_InitStructure);
		GPIO_ResetBits(GPIOF, CPAD_ADCPin);
		delay_us(20);
		GPIO_InitStructure.GPIO_Pin = CPAD_ADCPin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOF, &GPIO_InitStructure);
		delay_us(80);
		ADC_RegularChannelConfig(ADC3, CPAD_ADCChannel, 1, ADC_SampleTime_480Cycles);
		ADC_SoftwareStartConv(ADC3);
		while(!ADC_GetFlagStatus(ADC3, ADC_FLAG_EOC));
		sum += ADC_GetConversionValue(ADC3);
	}
	CPAD_threshold = sum / 10 - 2000;
}
/**
 * read touch pad value
 */
uint16_t getCPADValue(){
	GPIO_InitStructure.GPIO_Pin = CPAD_ADCPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOF, CPAD_ADCPin);
	delay_us(20);
	GPIO_InitStructure.GPIO_Pin = CPAD_ADCPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	delay_us(80);
	ADC_RegularChannelConfig(ADC3, CPAD_ADCChannel, 1, ADC_SampleTime_480Cycles);
	ADC_SoftwareStartConv(ADC3);
	while(!ADC_GetFlagStatus(ADC3, ADC_FLAG_EOC));
	uint16_t result = ADC_GetConversionValue(ADC3);
	LCD_ShowNum(30,80,result,4,16);
	return result;
}
int
main(int argc, char* argv[])

{
	uint16_t i = 0;
	SystemInit();
	delay_init(168);
	LCD_Init();
	LCD_ShowString(30,40,200,16,16,"Hello World");
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);
	CPADInit();
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);


	GPIO_WriteBit(GPIOF, GPIO_Pin_10, 0);
	GPIO_WriteBit(GPIOF, GPIO_Pin_9, 1);

  // At this stage the system clock should have already been configured
  // at high speed.


  // Infinite loop
  while (1)
    {
	  if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0){
		  while(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0);
		  i++;
		  switch(i % 24){
			  case 0:{
				  CPAD_ADCChannel = ADC_Channel_9;
				  CPAD_ADCPin = GPIO_Pin_3;
				  CPAD_MUXChannel = 3;
				  break;
			  }
			  case 1:{
				  CPAD_ADCChannel = ADC_Channel_9;
				  CPAD_ADCPin = GPIO_Pin_3;
				  CPAD_MUXChannel = 0;
				  break;
			  }
			  case 2:{
				  CPAD_ADCChannel = ADC_Channel_9;
			  	  CPAD_ADCPin = GPIO_Pin_3;
				  CPAD_MUXChannel = 1;
				  break;
			  }
			  case 3:{
				  CPAD_ADCChannel = ADC_Channel_9;
			  	  CPAD_ADCPin = GPIO_Pin_3;
				  CPAD_MUXChannel = 2;
				  break;
			  }
			  case 4:{
				  CPAD_ADCChannel = ADC_Channel_9;
			  	  CPAD_ADCPin = GPIO_Pin_3;
				  CPAD_MUXChannel = 5;
				  break;
			  }
			  case 5:{
				  CPAD_ADCChannel = ADC_Channel_9;
			  	  CPAD_ADCPin = GPIO_Pin_3;
				  CPAD_MUXChannel = 7;
				  break;
			  }
			  case 6:{
				  CPAD_ADCChannel = ADC_Channel_9;
			  	  CPAD_ADCPin = GPIO_Pin_3;
				  CPAD_MUXChannel = 6;
				  break;
			  }
			  case 7:{
				  CPAD_ADCChannel = ADC_Channel_9;
			  	  CPAD_ADCPin = GPIO_Pin_3;
				  CPAD_MUXChannel = 4;
				  break;
			  }
			  case 8:{
				  CPAD_ADCChannel = ADC_Channel_14;
			  	  CPAD_ADCPin = GPIO_Pin_4;
				  CPAD_MUXChannel = 3;
				  break;
			  }
			  case 9:{
				  CPAD_ADCChannel = ADC_Channel_14;
			  	  CPAD_ADCPin = GPIO_Pin_4;
				  CPAD_MUXChannel = 0;
				  break;
			  }
			  case 10:{
				  CPAD_ADCChannel = ADC_Channel_14;
			  	  CPAD_ADCPin = GPIO_Pin_4;
				  CPAD_MUXChannel = 1;
				  break;
			  }
			  case 11:{
				  CPAD_ADCChannel = ADC_Channel_14;
			  	  CPAD_ADCPin = GPIO_Pin_4;
				  CPAD_MUXChannel = 2;
				  break;
			  }
			  case 12:{
				  CPAD_ADCChannel = ADC_Channel_14;
			  	  CPAD_ADCPin = GPIO_Pin_4;
				  CPAD_MUXChannel = 5;
				  break;
			  }
			  case 13:{
				  CPAD_ADCChannel = ADC_Channel_14;
			  	  CPAD_ADCPin = GPIO_Pin_4;
				  CPAD_MUXChannel = 7;
				  break;
			  }
			  case 14:{
				  CPAD_ADCChannel = ADC_Channel_14;
			  	  CPAD_ADCPin = GPIO_Pin_4;
				  CPAD_MUXChannel = 6;
				  break;
			  }
			  case 15:{
				  CPAD_ADCChannel = ADC_Channel_14;
			  	  CPAD_ADCPin = GPIO_Pin_4;
				  CPAD_MUXChannel = 4;
				  break;
			  }
			  case 16:{
				  CPAD_ADCChannel = ADC_Channel_15;
			  	  CPAD_ADCPin = GPIO_Pin_5;
				  CPAD_MUXChannel = 3;
				  break;
			  }
			  case 17:{
				  CPAD_ADCChannel = ADC_Channel_15;
			  	  CPAD_ADCPin = GPIO_Pin_5;
				  CPAD_MUXChannel = 0;
				  break;
			  }
			  case 18:{
				  CPAD_ADCChannel = ADC_Channel_15;
			  	  CPAD_ADCPin = GPIO_Pin_5;
				  CPAD_MUXChannel = 1;
				  break;
			  }
			  case 19:{
				  CPAD_ADCChannel = ADC_Channel_15;
			  	  CPAD_ADCPin = GPIO_Pin_5;
				  CPAD_MUXChannel = 2;
				  break;
			  }
			  case 20:{
				  CPAD_ADCChannel = ADC_Channel_15;
			  	  CPAD_ADCPin = GPIO_Pin_5;
				  CPAD_MUXChannel = 5;
				  break;
			  }
			  case 21:{
				  CPAD_ADCChannel = ADC_Channel_15;
			  	  CPAD_ADCPin = GPIO_Pin_5;
				  CPAD_MUXChannel = 7;
				  break;
			  }
			  case 22:{
				  CPAD_ADCChannel = ADC_Channel_15;
			  	  CPAD_ADCPin = GPIO_Pin_5;
				  CPAD_MUXChannel = 6;
				  break;
			  }
			  case 23:{
				  CPAD_ADCChannel = ADC_Channel_15;
			  	  CPAD_ADCPin = GPIO_Pin_5;
				  CPAD_MUXChannel = 4;
				  break;
			  }
		  }
		  uint16_t dummy = GPIOF->ODR;
		  GPIOF->ODR = (dummy & 0xfff8) | (CPAD_MUXChannel);
		  CPADCalibrate();
		  LCD_ShowNum(30, 120, i % 24, 2, 16);
	  }


	  if(getCPADValue() > CPAD_threshold){
		  GPIO_WriteBit(GPIOF, GPIO_Pin_9, 1);
	  }
	  else{
		  GPIO_WriteBit(GPIOF, GPIO_Pin_9, 0);
	  }
    }
}
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

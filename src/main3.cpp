/*
 * main.cpp
 *
 *  Created on: 2016��5��4��
 *      Author: YU Chendi
 */

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
#include "CKey.h"
#include "timer_interrupt.h"
#include "system_stm32f4xx.h"
#include "sound.h"
#include "DAC.h"
//#include "extern.h"

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

static GPIO_InitTypeDef GPIO_InitStructure;
CKey* keys[24];
int i = 0;
extern int flag[8]={0};

int
main(int argc, char* argv[])
{
	//System init
	SystemInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init(168);
	//LCD init
	LCD_Init();
	LCD_ShowString(30,40,200,16,16,"Hello World");
	//Touch keys init
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);
	CKey_init();
	for (int i = 0; i < 24; i++){
		keys[i] = new CKey(i);
	}
	keys[0]->setMUX();
	LCD_ShowNum(30, 120, 0, 2, 16);
	//LED init
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
	//Button key1 init
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	 DAC_Init();
	 Buf_Init();
     Timer_Init(30,70);

//
//	GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_RESET);
//	GPIO_WriteBit(GPIOF, GPIO_Pin_9, Bit_SET);


  // At this stage the system clock should have already been configured
  // at high speed.
     // Buf_Clear(8);
    //  LCD_Fill(30, 150 + 3 * 25, 200, 175 + 3* 25, RED);
      //Buf_Clear(1);


//      Buf_Clear(4);

//      Buf_Clear(4);
//      delay_ms(400);
//      Buf_Clear(6);
//      delay_ms(400);
//      Buf_Clear(7);
//      delay_ms(400);
//      Buf_Clear(7);
//
//      delay_ms(200);
//      Buf_Clear(7);
//      delay_ms(520);
//      Buf_Clear(4);
//      delay_ms(375);
//      Buf_Clear(4);
//      delay_ms(350);



  //LCD_Fill(30, 150 + 3 * 25, 200, 175 + 3* 25, RED);



  // Infinite loop

  while (1)
    {
	  //This part tests whether each key is functional.
	  //LED0 lights up if a key is touched.
	  //Press key1 to move to the next key.
//	  if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0){
//		  while(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0);
//		  i++;
//		  LCD_ShowNum(30, 120, i % 24, 2, 16);
//		  keys[i % 24]->setMUX();
//	  }
//
//
//	  if(!keys[i % 24]->isPressed()){
//		  GPIO_WriteBit(GPIOF, GPIO_Pin_9, Bit_SET);
//	  }
//	  else{
//		  GPIO_WriteBit(GPIOF, GPIO_Pin_9, Bit_RESET);
//	  }

	 for (uint16_t i = 0; i < 24; i++){
		  keys[i]->setMUX();
		  delay_us(10);
		  if(!keys[i]->isPressed()){
			  LCD_Fill(30, 150 + i * 25, 200, 175 + i * 25, BLUE);
			  flag[i]=0;

		  }
		  else{
			  LCD_Fill(30, 150 + i * 25, 200, 175 + i * 25, RED);
			  if(flag[i] == 0 )
			  {Buf_Clear(i);flag[i]=1;}


		  }
	  }

    }

}
#pragma GCC diagnostic pop



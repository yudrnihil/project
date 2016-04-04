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
#include "SDCard.h"
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

int
main(int argc, char* argv[])

{
	//System init
	SystemInit();
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


	GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_RESET);
	GPIO_WriteBit(GPIOF, GPIO_Pin_9, Bit_SET);

	//SD Card init
	u32 sd_size;
	u8* buf = new u8[512];
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	while(SD_Init()){
	}
	LCD_ShowString(30,150,200,16,16,"SD Card OK    ");
	LCD_ShowString(30,170,200,16,16,"SD Card Size:     MB");
	LCD_ShowNum(30+13*8,170,SDCardInfo.CardCapacity>>20,5,16);//显示SD卡容量
	if(SD_ReadDisk(buf,0,1)==0)	//读取0扇区的内容
				{
		for(sd_size=0;sd_size<512;sd_size++)LCD_ShowNum(30+40*(sd_size%20),190+sd_size/20*20,buf[sd_size],3,16);
				}

  // At this stage the system clock should have already been configured
  // at high speed.


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


//	  for (uint16_t i = 0; i < 24; i++){
//		  keys[i]->setMUX();
//		  delay_us(10);
//		  if(!keys[i]->isPressed()){
//			  LCD_Fill(30, 150 + i * 25, 200, 175 + i * 25, BLUE);
//		  }
//		  else{
//			  LCD_Fill(30, 150 + i * 25, 200, 175 + i * 25, RED);
//		  }
//	  }
    }
}
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

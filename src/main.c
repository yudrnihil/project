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
#include "dac.h"

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

int
main(int argc, char* argv[])
{
	SystemInit();
	delay_init(168);
	LCD_Init();
	LCD_ShowString(30,40,200,16,16,"Hello World");
	DAC1_Init();
	RCC->AHB1ENR |= 1 << 5;
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

	GPIO_WriteBit(GPIOF, GPIO_Pin_10, 0);
	GPIO_WriteBit(GPIOF, GPIO_Pin_9, 1);
  // At this stage the system clock should have already been configured
  // at high speed.

  // Infinite loop
  while (1)
    {
	  GPIOF->ODR ^= 3<<9;
	  delay_ms(500);
    }
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

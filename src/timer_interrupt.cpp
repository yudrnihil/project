/*
 * timer_interrupt.cpp
 *
 *  Created on: 2016Äê4ÔÂ9ÈÕ
 *      Author: eric
 */
#include "timer_interrupt.h"
#include "sound.h"
#include "DAC.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_conf.h"

void Timer_Init(uint16_t arr,uint16_t psc){
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);

	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//arr
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //psc
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //count up
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//TIM3 init
    TIM_Cmd(TIM3,ENABLE);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //TIM3 interrupt init


	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void TIM3_IRQHandler(void){
	if(TIM_GetITStatus(TIM3,TIM_IT_Update) == SET){
    	Send2DAC(Next_Round());
		//IT function
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
}

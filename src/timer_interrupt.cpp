/*
 * timer_interrupt.cpp
 *
 *  Created on: 2016年4月9日
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

	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//初始化TIM3
    TIM_Cmd(TIM3,ENABLE);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断


	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
//	LCD_Fill(30, 150 + 1 * 25, 200, 175 + 1 * 25, RED);
}

void TIM3_IRQHandler(void){
	if(TIM_GetITStatus(TIM3,TIM_IT_Update) == SET){
    	Send2DAC(Next_Round());
		//IT function
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
}

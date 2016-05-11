/*
 * dac.cpp
 *
 *  Created on: 2016Äê4ÔÂ9ÈÕ
 *      Author: eric
 */

#include "DAC.h"
#include "stm32f4xx_dac.h"

void DAC_Init(){
	  GPIO_InitTypeDef  GPIO_InitStructure;
	  DAC_InitTypeDef DAC_InitType;

	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_4;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	  GPIO_Init(GPIOA, &GPIO_InitStructure);

	  DAC_InitType.DAC_Trigger=DAC_Trigger_None;
	  DAC_InitType.DAC_WaveGeneration=DAC_WaveGeneration_None;
	  DAC_InitType.DAC_LFSRUnmask_TriangleAmplitude=DAC_LFSRUnmask_Bit0;
	  DAC_InitType.DAC_OutputBuffer=DAC_OutputBuffer_Disable ;
	  DAC_Init(DAC_Channel_2,&DAC_InitType);
	  DAC_Init(DAC_Channel_1,&DAC_InitType);

	  DAC_Cmd(DAC_Channel_2, ENABLE);
	  DAC_Cmd(DAC_Channel_1, ENABLE);
	  DAC_SetDualChannelData(DAC_Align_12b_R, 0, 0);
}

uint16_t Send2DAC(uint16_t d_value){
	DAC_SetDualChannelData(DAC_Align_12b_R, d_value, d_value);
	return 0;
}

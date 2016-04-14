#include "key.h"
#include "delay.h" 

void KEY_Init(void)
{
	
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOE, ENABLE);
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4; //KEY0 KEY1 KEY2
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);//GPIOE2,3,4
	
	 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;//WK_UP
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);//GPIOA0
 
} 

/**
 * Key scan
 * @return
 * 0 not pressed
 * 1 key0 pressed
 * 2 key1 pressed
 * 3 key2 pressed
 * 4 WK_UP pressed
 */
u8 KEY_Scan(void)
{	 
	if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0){
		while(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0);
		return 2;
	}
	if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == 0){
		while(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == 0);
		return 1;
	}

	if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_2) == 0){
		while(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_2) == 0);
		return 3;
	}
	if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) != 0){
		while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) != 0);
		return 4;
	}
	return 0;
}





















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
#include "UI.h"
#include "key.h"
#include "fatfs/ff.h"
#include "SDCard.h"
#include "WAVPlayer.h"
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
int flag[24]={0};
uint8_t timbre;

u8 exf_getfree(char *drv,u32 *total,u32 *free)
{
	FATFS *fs1;
	u8 res;
    u32 fre_clust=0, fre_sect=0, tot_sect=0;
    //得到磁盘信息及空闲簇数量
    res =(u32)f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res==0)
	{
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//得到总扇区数
	    fre_sect=fre_clust*fs1->csize;			//得到空闲扇区数
#if _MAX_SS!=512				  				//扇区大小不是512字节,则转换为512字节
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif
		*total=tot_sect>>1;	//单位为KB
		*free=fre_sect>>1;	//单位为KB
 	}
	return res;
}


u8 mf_scan_files(char* path)
{
	FRESULT res;
	FILINFO fno;
	DIR dir;
	u16 count = 0;;
    char *fn;   /* This function is assuming non-Unicode cfg. */
#if _USE_LFN
 	fno.lfsize = _MAX_LFN * 2 + 1;
	fno.lfname = new char[fno.lfsize];
#endif

    res = f_opendir(&dir,(const TCHAR*)path); //打开一个目录
    if (res == FR_OK)
	{
		while(1)
		{
	        res = f_readdir(&dir, &fno);                   //读取目录下的一个文件
	        if (res != FR_OK || fno.fname[0] == 0) break;  //错误了/到末尾了,退出
	        //if (fileinfo.fname[0] == '.') continue;             //忽略上级目录
#if _USE_LFN
        	fn = *fno.lfname ? fno.lfname : fno.fname;
#else
        	fn = fno.fname;
#endif	                                              /* It is a file. */
			//LCD_ShowString(30, 250 + count*30, 200, 16, 16, path);//打印路径
			LCD_ShowString(50, 250 + count*30, 200, 16, 16, fn);//打印文件名
			count++;
		}
    }
    f_closedir(&dir);
    return res;
}

int
main(int argc, char* argv[])
{
	//System init
	SystemInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init(168);
	//key init
	KEY_Init();
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
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//
	//	GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_RESET);
	//	GPIO_WriteBit(GPIOF, GPIO_Pin_9, Bit_SET);




  // At this stage the system clock should have already been configured
  // at high speed.

//	    	 DAC_Init();
//	    	 Buf_Init();
//	         Timer_Init(30,70);


     uint8_t mode = modeSelect();
     if (mode == 0){
    	 DAC_Init();
    	 Buf_Init();
         Timer_Init(30,70);

         while(1){
			 timbre = timbreSelect();
			while(1){
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
					  {Buf_Clear(23-i);flag[i]=1;}
				  }
				}
				if(KEY_Scan()!=0){
					LCD_Clear(WHITE);
					LCD_ShowString(30,40,200,16,16,"Hello World");
					LCD_ShowNum(30, 120, 0, 2, 16);
					break;
				}
			}
         }
     }
     else{
    	 //SD Card init
    	 u32 sd_size;
    	 u8* buf = new u8[512];
    	 NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    	 while(SD_Init()){
    	 }
    	 FATFS* fs = new FATFS;
    	 FIL* file = new FIL;
    	 FIL* ftemp = new FIL;
    	 u8* fatbuf = new u8[512];
    	 u32 total, free;
    	 f_mount(fs,"0:",1);
    	 exf_getfree("0", &total, &free);
    	 LCD_ShowString(30,150,200,16,16,"FATFS OK!");
    	 LCD_ShowString(30,170,200,16,16,"SD Total Size:     MB");
    	 LCD_ShowString(30,190,200,16,16,"SD  Free Size:     MB");
    	 LCD_ShowNum(30+8*14,170,total>>10,5,16);				//显示SD卡总容量 MB
    	 LCD_ShowNum(30+8*14,190,free>>10,5,16);					//显示SD卡剩余容量 MB
    	 mf_scan_files("0:");
    	 //wav_play_song("0:/fox.wav");
    	 wavController("0:");
     }
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
//	  		  keys[i]->setMUX();
//	  		  delay_us(10);
//	  		  if(!keys[i]->isPressed()){
//	  			  LCD_Fill(30, 150 + i * 25, 200, 175 + i * 25, BLUE);
//	  			  flag[i]=0;
//
//	  		  }
//	  		  else{
//	  			  LCD_Fill(30, 150 + i * 25, 200, 175 + i * 25, RED);
//	  			  if(flag[i] == 0 )
//	  			  {Buf_Clear(23-i);flag[i]=1;}
//
//
//	  		  }
//	  }



    }

}
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

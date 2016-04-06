//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "diag/Trace.h"
#include "stm32f4xx.h"
#include "delay.h"
#include "lcd.h"
#include "CKey.h"
#include "SDCard.h"
#include "fatfs/ff.h"
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

u8 exf_getfree(char *drv,u32 *total,u32 *free)
{
	FATFS *fs1;
	u8 res;
    u32 fre_clust=0, fre_sect=0, tot_sect=0;
    //�õ�������Ϣ�����д�����
    res =(u32)f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res==0)
	{
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//�õ���������
	    fre_sect=fre_clust*fs1->csize;			//�õ�����������
#if _MAX_SS!=512				  				//������С����512�ֽ�,��ת��Ϊ512�ֽ�
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif
		*total=tot_sect>>1;	//��λΪKB
		*free=fre_sect>>1;	//��λΪKB
 	}
	return res;
}

u8 mf_scan_files(char* path)
{
	FRESULT res;
	FILINFO fno;
	DIR dir;
	u16 i;
    char *fn;   /* This function is assuming non-Unicode cfg. */
#if _USE_LFN
 	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = mymalloc(SRAMIN,fileinfo.lfsize);
#endif

    res = f_opendir(&dir,(const TCHAR*)path); //��һ��Ŀ¼
    if (res == FR_OK)
	{
    	i = strlen(path);
		while(1)
		{
	        res = f_readdir(&dir, &fno);                   //��ȡĿ¼�µ�һ���ļ�
	        if (res != FR_OK || fno.fname[0] == 0) break;  //������/��ĩβ��,�˳�
	        //if (fileinfo.fname[0] == '.') continue;             //�����ϼ�Ŀ¼
#if _USE_LFN
        	fn = *fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;
#else
        	fn = fno.fname;
#endif	                                              /* It is a file. */
			LCD_ShowString(30, 250, 200, 16, 16, path);//��ӡ·��
			LCD_ShowString(30, 250, 250, 16, 16, fn);//��ӡ�ļ���
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
//	LCD_ShowString(30,150,200,16,16,"SD Card OK    ");
//	LCD_ShowString(30,170,200,16,16,"SD Card Size:     MB");
//	LCD_ShowNum(30+13*8,170,SDCardInfo.CardCapacity>>20,5,16);//��ʾSD������
//	if(SD_ReadDisk(buf,0,1)==0){	//��ȡ0����������
//		for(sd_size=0;sd_size<512;sd_size++){
//			LCD_ShowNum(30+40*(sd_size%20),190+sd_size/20*20,buf[sd_size],3,16);
//		}
//	}
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
	LCD_ShowNum(30+8*14,170,total>>10,5,16);				//��ʾSD�������� MB
	LCD_ShowNum(30+8*14,190,free>>10,5,16);					//��ʾSD��ʣ������ MB
	mf_scan_files("0:");

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

/*
 * WAVPlayer.cpp
 *
 *  Created on: 2016��4��11��
 *      Author: YU Chendi
 */

#include "wavPlayer.h"
#include "delay.h"
#include "fatfs/ff.h"
#include "stm32f4xx_it.h"
//////////////////////////////////////////////////////////////////////////////////
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//WAV �������
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/6/29
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved
//********************************************************************************
//V1.0 ˵��
//1,֧��16λ/24λWAV�ļ�����
//2,��߿���֧�ֵ�192K/24bit��WAV��ʽ.
//////////////////////////////////////////////////////////////////////////////////

/**
 * Assume 8bit mono channel
 */

__wavctrl wavctrl;		//WAV���ƽṹ��
vu8 wavtransferend=0;	//dma transfer end
vu8 wavdatabuf=0;		//dma which buf to use

NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
DMA_InitTypeDef DMA_InitStructure;
DAC_InitTypeDef DAC_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;

__audiodev audiodev;

//WAV������ʼ��
//fname:�ļ�·��+�ļ���
//wavx:wav ��Ϣ��Žṹ��ָ��
//����ֵ:0,�ɹ�;1,���ļ�ʧ��;2,��WAV�ļ�;3,DATA����δ�ҵ�.
u8 wav_decode_init(char* fname,__wavctrl* wavx)
{
	FIL*ftemp;
	u8 *buf;
	u32 br=0;
	u8 res=0;

	ChunkRIFF *riff;
	ChunkFMT *fmt;
	ChunkFACT *fact;
	ChunkDATA *data;
	ftemp= new FIL;
	//buf=mymalloc(SRAMIN,512);
	buf = new u8[512];
	if(ftemp&&buf)	//�ڴ�����ɹ�
	{
		res=f_open(ftemp,(TCHAR*)fname,FA_READ);//���ļ�
		if(res==FR_OK)
		{
			f_read(ftemp,buf,512,&br);	//��ȡ512�ֽ�������
			riff=(ChunkRIFF *)buf;		//��ȡRIFF��
			if(riff->Format==0X45564157)//��WAV�ļ�
			{
				fmt=(ChunkFMT *)(buf+12);	//��ȡFMT��
				fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);//��ȡFACT��
				if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;//����fact/LIST���ʱ��(δ����)
				else wavx->datastart=12+8+fmt->ChunkSize;
				data=(ChunkDATA *)(buf+wavx->datastart);	//��ȡDATA��
				if(data->ChunkID==0X61746164)//�����ɹ�!
				{
					wavx->audioformat=fmt->AudioFormat;		//��Ƶ��ʽ
					wavx->nchannels=fmt->NumOfChannels;		//ͨ����
					wavx->samplerate=fmt->SampleRate;		//������
					wavx->bitrate=fmt->ByteRate*8;			//�õ�λ��
					wavx->blockalign=fmt->BlockAlign;		//�����
					wavx->bps=fmt->BitsPerSample;			//λ��,16/24/32λ

					wavx->datasize=data->ChunkSize;			//���ݿ��С
					wavx->datastart=wavx->datastart+8;		//��������ʼ�ĵط�.

//					printf("wavx->audioformat:%d\r\n",wavx->audioformat);
//					printf("wavx->nchannels:%d\r\n",wavx->nchannels);
//					printf("wavx->samplerate:%d\r\n",wavx->samplerate);
//					printf("wavx->bitrate:%d\r\n",wavx->bitrate);
//					printf("wavx->blockalign:%d\r\n",wavx->blockalign);
//					printf("wavx->bps:%d\r\n",wavx->bps);
//					printf("wavx->datasize:%d\r\n",wavx->datasize);
//					printf("wavx->datastart:%d\r\n",wavx->datastart);

					TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);

					TIM_TimeBaseStructure.TIM_Period = 1000000/fmt->SampleRate - 1;
					TIM_TimeBaseStructure.TIM_Prescaler = 84;
					TIM_TimeBaseStructure.TIM_ClockDivision = 0;
					TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
					TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
					TIM_Cmd(TIM6, DISABLE);


				}else res=3;//data����δ�ҵ�.
			}else res=2;//��wav�ļ�

		}else res=1;//���ļ�����
	}

	f_close(ftemp);
	//myfree(SRAMIN,ftemp);//�ͷ��ڴ�
	delete ftemp;
	//myfree(SRAMIN,buf);
	delete [] buf;
	return 0;
}

//���buf
//buf:������
//size:���������
//bits:λ��(16/24)
//����ֵ:���������ݸ���
u32 wav_buffill(u8 *buf,u16 size,u8 bits)
{
	u16 readlen=0;
	u32 bread;
	u16 i;
	u8 *p;
	if(bits==24)//24bit��Ƶ,��Ҫ����һ��
	{
		readlen=(size/4)*3;							//�˴�Ҫ��ȡ���ֽ���
		f_read(audiodev.file,audiodev.tbuf,readlen,(UINT*)&bread);	//��ȡ����
		p=audiodev.tbuf;
		for(i=0;i<size;)
		{
			buf[i++]=p[1];
			buf[i]=p[2];
			i+=2;
			buf[i++]=p[0];
			p+=3;
		}
		bread=(bread*4)/3;		//����Ĵ�С.
	}else
	{
		f_read(audiodev.file,buf,size,(UINT*)&bread);//16bit��Ƶ,ֱ�Ӷ�ȡ����
		if(bread<size)//����������,����0
		{
			for(i=bread;i<size-bread;i++)buf[i]=0;
		}
	}
	return bread;
}
//WAV����ʱ,I2S DMA����ص�����
void tx_callback(void)
{
	u16 i;
	if(DMA1_Stream5->CR&(1<<19))
	{
		wavdatabuf=0;
		if((audiodev.status&0X01)==0)
		{
			for(i=0;i<WAV_DAC_TX_DMA_BUFSIZE;i++)//��ͣ
			{
				audiodev.dacbuf1[i]=0;//���0
			}
		}
	}else
	{
		wavdatabuf=1;
		if((audiodev.status&0X01)==0)
		{
			for(i=0;i<WAV_DAC_TX_DMA_BUFSIZE;i++)//��ͣ
			{
				audiodev.dacbuf2[i]=0;//���0
			}
		}
	}
	wavtransferend=1;
}
//�õ���ǰ����ʱ��
//fx:�ļ�ָ��
//wavx:wav���ſ�����
void wav_get_curtime(FIL*fx,__wavctrl *wavx)
{
	long long fpos;
 	wavx->totsec=wavx->datasize/(wavx->bitrate/8);	//�����ܳ���(��λ:��)
	fpos=fx->fptr-wavx->datastart; 					//�õ���ǰ�ļ����ŵ��ĵط�
	wavx->cursec=fpos*wavx->totsec/wavx->datasize;	//��ǰ���ŵ��ڶ�������?
}
//����ĳ��WAV�ļ�
//fname:wav�ļ�·��.
//����ֵ:
//KEY0_PRES:��һ��
//KEY1_PRES:��һ��
//����:����
u8 wav_play_song(char* fname)
{
	u8 key;
	u8 t=0;
	u8 res;
	u32 fillnum;
	audiodev.file=new FIL;
	audiodev.dacbuf1 = new u8[512];
	audiodev.dacbuf2 = new u8[512];
	audiodev.tbuf = new u8[512];
	DAC_WAV_Init(audiodev.dacbuf1, audiodev.dacbuf2, 512);
	if(audiodev.file&&audiodev.dacbuf1&&audiodev.dacbuf2&&audiodev.tbuf)
	{
		res=wav_decode_init(fname,&wavctrl);//�õ��ļ�����Ϣ
		if(res==0)//�����ļ��ɹ�
		{
			//if(wavctrl.bps==16)
			if(wavctrl.bps == 8)
			{
				//WM8978_I2S_Cfg(2,0);	//�����ֱ�׼,16λ���ݳ���
				//I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_16bextended);		//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,16λ��չ֡����
				TIM_Cmd(TIM6, ENABLE);
//			}else if(wavctrl.bps==24)
//			{
//				WM8978_I2S_Cfg(2,2);	//�����ֱ�׼,24λ���ݳ���
//				I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_24b);		//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,24λ��չ֡����
//			}
			//I2S2_SampleRate_Set(wavctrl.samplerate);//���ò�����
			//I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE/2); //����TX DMA
			//i2s_tx_callback=wav_i2s_dma_tx_callback;			//�ص�����ָwav_i2s_dma_callback
			//audio_stop();
			res=f_open(audiodev.file,(TCHAR*)fname,FA_READ);	//���ļ�
			if(res==0)
			{
				f_lseek(audiodev.file, wavctrl.datastart);		//�����ļ�ͷ
				fillnum=wav_buffill(audiodev.dacbuf1,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
				fillnum=wav_buffill(audiodev.dacbuf2,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
				audiodev.status=3<<0;
				while(res==0)
				{
//					while(DMA_GetFlagStatus(DMA1_Stream5, DMA_FLAG_TCIF5) == 0);
//					tx_callback();
//					DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5);
					while(wavtransferend==0);//�ȴ�wav�������;
					wavtransferend=0;
					if(fillnum!=WAV_DAC_TX_DMA_BUFSIZE)//���Ž���?
					{
						//res=KEY0_PRES;
						break;
					}
 					if(wavdatabuf)fillnum=wav_buffill(audiodev.dacbuf2,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);//���buf2
					else fillnum=wav_buffill(audiodev.dacbuf1,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);//���buf1
					while(1)
					{
//						key=KEY_Scan(0);
//						if(key==WKUP_PRES)//��ͣ
//						{
//							if(audiodev.status&0X01)audiodev.status&=~(1<<0);
//							else audiodev.status|=0X01;
//						}
//						if(key==KEY2_PRES||key==KEY0_PRES)//��һ��/��һ��
//						{
//							res=key;
//							break;
//						}
//						wav_get_curtime(audiodev.file,&wavctrl);//�õ���ʱ��͵�ǰ���ŵ�ʱ��
//						audio_msg_show(wavctrl.totsec,wavctrl.cursec,wavctrl.bitrate);
//						t++;
//						if(t==20)
//						{
//							t=0;
// 							LED0=!LED0;
//						}
						if((audiodev.status&0X01)==0)delay_ms(10);
						else break;
					}
				}
				//audio_stop();
				audiodev.status = 0;
				DMA_DoubleBufferModeCmd(DMA1_Stream5, DISABLE);
			}else res=0XFF;
		}else res=0XFF;
	}else res=0XFF;
//	myfree(SRAMIN,audiodev.tbuf);	//�ͷ��ڴ�
//	myfree(SRAMIN,audiodev.i2sbuf1);//�ͷ��ڴ�
//	myfree(SRAMIN,audiodev.i2sbuf2);//�ͷ��ڴ�
//	myfree(SRAMIN,audiodev.file);	//�ͷ��ڴ�
	delete [] audiodev.tbuf;
	delete [] audiodev.dacbuf1;
	delete [] audiodev.dacbuf2;
	delete audiodev.file;
	return res;
	}
}

void DAC_WAV_Init(u8* buf0, u8* buf1, u16 num){

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6 | RCC_APB1Periph_DAC, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/**
	 * Use TIM6 as DAC trigger
	 */
//	TIM_TimeBaseStructure.TIM_Period = 22;
//	TIM_TimeBaseStructure.TIM_Prescaler = 0;
//	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
//	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
//	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
//	TIM_Cmd(TIM6, DISABLE);
//
//	TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);

	//DMA Init
	DMA_DeInit(DMA1_Stream5);
	DMA_InitStructure.DMA_Channel = DMA_Channel_7;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&DAC->DHR8R1;
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)buf0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_BufferSize = num;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Cmd(DMA1_Stream5, DISABLE);
	while (DMA1_Stream5->CR & DMA_SxCR_EN);
	DMA_Init(DMA1_Stream5, &DMA_InitStructure);


	DMA_DoubleBufferModeConfig(DMA1_Stream5, (u32)buf1, DMA_Memory_0);
	DMA_ITConfig(DMA1_Stream5, DMA_IT_TC, ENABLE);
	DMA_DoubleBufferModeCmd(DMA1_Stream5, ENABLE);
	DMA_Cmd(DMA1_Stream5, ENABLE);


	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;//��ռ���ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;//�����ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
    NVIC_Init(&NVIC_InitStructure);

	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_1, ENABLE);
	DAC_DMACmd(DAC_Channel_1, ENABLE);
}

void DMA1_Stream5_IRQHandler(void){
	if(DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5) == SET){
		DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5);
		tx_callback();
	}
}


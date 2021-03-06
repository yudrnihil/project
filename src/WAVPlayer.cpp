/*
 * WAVPlayer.cpp
 *
 *  Created on: 2016年4月11日
 *      Author: YU Chendi
 */

#include "wavPlayer.h"
#include "delay.h"
#include "fatfs/ff.h"
#include "stm32f4xx_it.h"
#include "lcd.h"

/**
 * WAV Player
 * 2016.04.13 YU CHENDI
 * Play WAV file from FATFS SD Card.
 * Assume 8 bit mono file format at this moment.
 * Based on Alientek wav player and AN3126 application note
 */
__wavctrl wavctrl;		//WAV control
vu8 wavtransferend=0;	//dma transfer end
vu8 wavdatabuf=0;		//dma which buf to use

NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
DMA_InitTypeDef DMA_InitStructure;
DAC_InitTypeDef DAC_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;

__audiodev audiodev;

u8 exf_getfree(char *drv,u32 *total,u32 *free)
{
	FATFS *fs1;
	u8 res;
    u32 fre_clust=0, fre_sect=0, tot_sect=0;
    //disk info
    res =(u32)f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res==0)
	{
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//sector size
	    fre_sect=fre_clust*fs1->csize;			//free sector
#if _MAX_SS!=512				  				//convert to 512B
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif
		*total=tot_sect>>1;	//in kb
		*free=fre_sect>>1;	//in kb
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
	        res = f_readdir(&dir, &fno);                   //next file in the directory
	        if (res != FR_OK || fno.fname[0] == 0) break;  //error/end
#if _USE_LFN
        	fn = *fno.lfname ? fno.lfname : fno.fname;
#else
        	fn = fno.fname;
#endif	                                              /* It is a file. */
			LCD_ShowString(50, 250 + count*30, 200, 16, 16, fn);//print file name
			count++;
		}
    }
    f_closedir(&dir);
    return res;
}

//WAV decode init
//fname:file path+name
//wavx:pointer to wavctrl
//return:0,success;1,cannot open file;2,not a wav;3,cannot find DATA.
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

	buf = new u8[512];
	if(ftemp&&buf)	//successful memory allocation
	{
		res=f_open(ftemp,(TCHAR*)fname,FA_READ);//open the file
		if(res==FR_OK)
		{
			f_read(ftemp,buf,512,&br);	//read 512 bytes
			riff=(ChunkRIFF *)buf;		//read RIFF
			if(riff->Format==0X45564157)//is a WAV
			{
				fmt=(ChunkFMT *)(buf+12);	//read FMT
				fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);//read FACT
				if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;
				else wavx->datastart=12+8+fmt->ChunkSize;
				data=(ChunkDATA *)(buf+wavx->datastart);	//read DATA
				if(data->ChunkID==0X61746164)//decode success
				{
					wavx->audioformat=fmt->AudioFormat;
					wavx->nchannels=fmt->NumOfChannels;
					wavx->samplerate=fmt->SampleRate;
					wavx->bitrate=fmt->ByteRate*8;
					wavx->blockalign=fmt->BlockAlign;
					wavx->bps=fmt->BitsPerSample;			//Supports 8 bit only

					wavx->datasize=data->ChunkSize;
					wavx->datastart=wavx->datastart+8;

					//init TIM6, set sample rate
					//connect TIM6 to DAC trigger
					TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
					TIM_TimeBaseStructure.TIM_Period = 1000000/fmt->SampleRate - 1;
					TIM_TimeBaseStructure.TIM_Prescaler = 84;
					TIM_TimeBaseStructure.TIM_ClockDivision = 0;
					TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
					TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
					TIM_Cmd(TIM6, DISABLE);

				}else res=3;//DATA not found
			}else res=2;//not a WAV

		}else res=1;//cannot open file
	}

	f_close(ftemp);
	delete ftemp;
	delete [] buf;
	return res;
}

//fill buffer
//buf: data buffer
//size: number of bytes to fill
//bits: bits per sample, 8
//return: number of bytes filled
u32 wav_buffill(u8 *buf,u16 size,u8 bits)
{
	u32 bread;
	u16 i;
		f_read(audiodev.file,buf,size,(UINT*)&bread);
		if(bread<size)//pad 0
		{
			for(i=bread;i<size-bread;i++)buf[i]=0;
		}
	return bread;
}

/**
 * play a wav file
 * @param fname path and name of the wav file
 * @return 0 when finish playing
 *         1,3 next song
 *         0xff error
 */
u8 wav_play_song(char* fname)
{
	u8 key;
	u8 res;
	u32 fillnum;
	if(audiodev.file&&audiodev.dacbuf1&&audiodev.dacbuf2)
	{
		res=wav_decode_init(fname,&wavctrl);//file info
		if(res==0)//decode success
		{
			if(wavctrl.bps == 8)
			{
				res=f_open(audiodev.file,(TCHAR*)fname,FA_READ);	//open file
				if(res==0)
				{
					f_lseek(audiodev.file, wavctrl.datastart);		//skip file head
					fillnum=wav_buffill(audiodev.dacbuf1,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
					fillnum=wav_buffill(audiodev.dacbuf2,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
					audiodev.status=3<<0;

					//config and enalbe DMA
					DMA_Reconfigure(wavctrl.nchannels);
					DMA_Cmd(DMA1_Stream5, ENABLE);

					//enable TIM6, start playing
					TIM_Cmd(TIM6, ENABLE);
					while(res==0)
					{
						while(wavtransferend==0);//wait for completing transfer
						wavtransferend=0;
						if(fillnum!=WAV_DAC_TX_DMA_BUFSIZE)//end of the song?
						{
							break;
						}
						if(wavdatabuf)fillnum=wav_buffill(audiodev.dacbuf2,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);//fill buffer2
						else fillnum=wav_buffill(audiodev.dacbuf1,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);//fill buffer1
						while(1)
						{
							key=KEY_Scan();
							if(key == 2)//press key 2 to pause
							{
								if(audiodev.status&0X01){
									audiodev.status&=~(1<<0);
									TIM_Cmd(TIM6, DISABLE);
								}
								else {
									audiodev.status|=0X01;
									TIM_Cmd(TIM6, ENABLE);
								}

							}
							if(key == 1 ||key == 3)//press key1 or 3, next song
							{
								res=key;
								break;
							}
							if((audiodev.status&0X01)==0){
								delay_ms(10);
							}
							else break;
						}
					}
					audiodev.status = 0;
					//disable TIM6, stop playing
					TIM_Cmd(TIM6, DISABLE);
					//disable DMA
					DMA_Cmd(DMA1_Stream5, DISABLE);
				}else res=0XFF; //err: cannot open file
			}else res=0XFF; //err: wav file is not 8bit, not supported yet
		}else res=0XFF; //err: cannot get file info
	}
	else{
		res = 0xff; //err: cannot allocate memory
	}
	return res;
}

//helper function: append path to file name
//eg: b = "fox.wav", return value = "0:\fox.wav"
char* append(char* b){
	char* result = new char[_MAX_LFN * 2 + 1];
	result[0] = '0';
	result[1] = ':';
	result[2] = '\\';
	u8 i = 0;
	while(b[i] != 0){
		result[i + 3] = b[i];
		i++;
	}
	result[i + 3] = 0;
	return result;
}

/**
 * controller, controls which song to play
 */
void wavController(char* path){
	FRESULT res;
	FILINFO fno;
	DIR dir;
	u8 wavStatus;
	char *fn;

	audiodev.file=new FIL;
	audiodev.dacbuf1 = new u8[512];
	audiodev.dacbuf2 = new u8[512];
	//init DAC, DMA
	DAC_WAV_Init(audiodev.dacbuf1, audiodev.dacbuf2, 256);

	fno.lfsize = _MAX_LFN * 2 + 1;
	fno.lfname = new char[fno.lfsize];
	res = f_opendir(&dir,(const TCHAR*)path);
	if(res == FR_OK){
		res = f_readdir(&dir, &fno);
		if(res != FR_OK || fno.fname[0] == 0){
			return;
		}
		fn = *fno.lfname ? fno.lfname : fno.fname;
		while(1){
			wavStatus = wav_play_song(append(fn)); //play a song
			if(wavStatus != 0){ //next song
				res = f_readdir(&dir, &fno);
				if(res != FR_OK || fno.fname[0] == 0){
					f_opendir(&dir, (const TCHAR*)path);
				}
				else{
					fn = *fno.lfname ? fno.lfname : fno.fname;
				}
			}
		}

	}
	delete [] audiodev.dacbuf1;
	delete [] audiodev.dacbuf2;
	delete audiodev.file;
}

/**
 * Init DAC, DMA, PA4
 * @param buf0 dma data buffer 0
 * @param buf1 dma data buffer 1
 * @param num buffer size in byte
 */
void DAC_WAV_Init(u8* buf0, u8* buf1, u16 num){
	//Clock enable
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6 | RCC_APB1Periph_DAC, ENABLE);

	//Channel1 init
	//PA4 init
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//DMA Init
	DMA_DeInit(DMA1_Stream5);
	DMA_InitStructure.DMA_Channel = DMA_Channel_7;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&DAC->DHR8RD;
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)buf0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_BufferSize = num;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
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

	//enable transfer complete interrupt
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;//抢占优先级0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;//子优先级0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
    NVIC_Init(&NVIC_InitStructure);

    //dac init
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_1, ENABLE);
	DAC_DMACmd(DAC_Channel_1, ENABLE);

	//Channel 2 init
	//PA4 init
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

    //dac init
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
	DAC_Init(DAC_Channel_2, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_2, ENABLE);
	DAC_DMACmd(DAC_Channel_2, ENABLE);
}

/**
 * DMA1_Stream5_IRQHandler
 * called upon transfer completion
 * set the buffer currently in use
 */
void DMA1_Stream5_IRQHandler(void){
	if(DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5) == SET){
		DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5);
		if(DMA1_Stream5->CR&(1<<19))
		{
			wavdatabuf=0;
		}else
		{
			wavdatabuf=1;
		}
		wavtransferend=1;
	}
}

/**
 * reconfigure DMA data size according to number of channels
 * channel: 1: mono; 2: dual
 */
void DMA_Reconfigure(uint8_t channel){
	DMA_InitStructure.DMA_BufferSize = channel == 1 ? WAV_DAC_TX_DMA_BUFSIZE : WAV_DAC_TX_DMA_BUFSIZE/2;
	DMA_InitStructure.DMA_MemoryDataSize = channel == 1 ? DMA_MemoryDataSize_Byte : DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_PeripheralDataSize = channel == 1 ? DMA_PeripheralDataSize_Byte : DMA_PeripheralDataSize_HalfWord;
	DMA_Cmd(DMA1_Stream5, DISABLE);
	while (DMA1_Stream5->CR & DMA_SxCR_EN);
	DMA_Init(DMA1_Stream5, &DMA_InitStructure);

	DMA_ITConfig(DMA1_Stream5, DMA_IT_TC, ENABLE);
	DMA_DoubleBufferModeCmd(DMA1_Stream5, ENABLE);

	DAC_DMACmd(DAC_Channel_1, ENABLE);
	DAC_DMACmd(DAC_Channel_2, ENABLE);
}

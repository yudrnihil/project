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

/**
 * WAV Player
 * 2016.04.13 YU CHENDI
 * Play WAV file from FATFS SD Card.
 * Assume 8 bit mono file format at this moment.
 * Based on Alientek wav player and AN3126 application note
 */
__wavctrl wavctrl;		//WAV控制结构体
vu8 wavtransferend=0;	//dma transfer end
vu8 wavdatabuf=0;		//dma which buf to use

NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
DMA_InitTypeDef DMA_InitStructure;
DAC_InitTypeDef DAC_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;

__audiodev audiodev;

//WAV解析初始化
//fname:文件路径+文件名
//wavx:wav 信息存放结构体指针
//返回值:0,成功;1,打开文件失败;2,非WAV文件;3,DATA区域未找到.
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
	if(ftemp&&buf)	//内存申请成功
	{
		res=f_open(ftemp,(TCHAR*)fname,FA_READ);//打开文件
		if(res==FR_OK)
		{
			f_read(ftemp,buf,512,&br);	//读取512字节在数据
			riff=(ChunkRIFF *)buf;		//获取RIFF块
			if(riff->Format==0X45564157)//是WAV文件
			{
				fmt=(ChunkFMT *)(buf+12);	//获取FMT块
				fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);//读取FACT块
				if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;//具有fact/LIST块的时候(未测试)
				else wavx->datastart=12+8+fmt->ChunkSize;
				data=(ChunkDATA *)(buf+wavx->datastart);	//读取DATA块
				if(data->ChunkID==0X61746164)//解析成功!
				{
					wavx->audioformat=fmt->AudioFormat;		//音频格式
					wavx->nchannels=fmt->NumOfChannels;		//通道数
					wavx->samplerate=fmt->SampleRate;		//采样率
					wavx->bitrate=fmt->ByteRate*8;			//得到位速
					wavx->blockalign=fmt->BlockAlign;		//块对齐
					wavx->bps=fmt->BitsPerSample;			//位数,8/16/32

					wavx->datasize=data->ChunkSize;			//数据块大小
					wavx->datastart=wavx->datastart+8;		//数据流开始的地方.

					//init TIM6, set sample rate
					//connect TIM6 to DAC trigger
					TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
					TIM_TimeBaseStructure.TIM_Period = 1000000/fmt->SampleRate - 1;
					TIM_TimeBaseStructure.TIM_Prescaler = 84;
					TIM_TimeBaseStructure.TIM_ClockDivision = 0;
					TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
					TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
					TIM_Cmd(TIM6, DISABLE);

				}else res=3;//data区域未找到.
			}else res=2;//非wav文件

		}else res=1;//打开文件错误
	}

	f_close(ftemp);
	delete ftemp;
	delete [] buf;
	return res;
}

//填充buf
//buf:数据区
//size:填充数据量
//bits:位数  8 at this moment
//返回值:读到的数据个数
u32 wav_buffill(u8 *buf,u16 size,u8 bits)
{
	u32 bread;
	u16 i;
	//u8 *p;
//	if(bits==24)//24bit音频,需要处理一下
//	{
//		readlen=(size/4)*3;							//此次要读取的字节数
//		f_read(audiodev.file,audiodev.tbuf,readlen,(UINT*)&bread);	//读取数据
//		p=audiodev.tbuf;
//		for(i=0;i<size;)
//		{
//			buf[i++]=p[1];
//			buf[i]=p[2];
//			i+=2;
//			buf[i++]=p[0];
//			p+=3;
//		}
//		bread=(bread*4)/3;		//填充后的大小.
//	}else
//	{
		f_read(audiodev.file,buf,size,(UINT*)&bread);//16bit音频,直接读取数据
		if(bread<size)//不够数据了,补充0
		{
			for(i=bread;i<size-bread;i++)buf[i]=0;
		}
//	}
	return bread;
}

//u32 wav_buffill(u8* bufCh0, u8* bufCh1, u16 size, u8 bits){
//	u32 bread;
//	u16 i;
//	u8 *p;
//
//	f_read(audiodev.file, audiodev.tbuf, size * 2, (UINT*)&bread);
//	p = audiodev.tbuf;
//	for(i = 0; i < size * 2 ; i += 2){
//		bufCh0[i] = p[i];
//		bufCh1[i] = p[i+1];
//	}
//	return bread;
//}

//得到当前播放时间
//fx:文件指针
//wavx:wav播放控制器
void wav_get_curtime(FIL*fx,__wavctrl *wavx)
{
	long long fpos;
 	wavx->totsec=wavx->datasize/(wavx->bitrate/8);	//歌曲总长度(单位:秒)
	fpos=fx->fptr-wavx->datastart; 					//得到当前文件播放到的地方
	wavx->cursec=fpos*wavx->totsec/wavx->datasize;	//当前播放到第多少秒了?
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
	u8 t=0;
	u8 res;
	u32 fillnum;
	if(audiodev.file&&audiodev.dacbuf1&&audiodev.dacbuf2&&audiodev.tbuf)
	{
		res=wav_decode_init(fname,&wavctrl);//得到文件的信息
		if(res==0)//解析文件成功
		{
			if(wavctrl.bps == 8)
			{
				res=f_open(audiodev.file,(TCHAR*)fname,FA_READ);	//打开文件
				if(res==0)
				{
					f_lseek(audiodev.file, wavctrl.datastart);		//跳过文件头
//					if(wavctrl.nchannels == 1){
						fillnum=wav_buffill(audiodev.dacbuf1,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
						fillnum=wav_buffill(audiodev.dacbuf2,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
						audiodev.status=3<<0;

						DMA_Cmd(DMA1_Stream5, ENABLE);
//					}
//					else{
//						fillnum=wav_buffill(audiodev.dacbuf1,audiodev.dacbuf3,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
//						fillnum=wav_buffill(audiodev.dacbuf2,audiodev.dacbuf4,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
//						fillnum = fillnum / 2;
//						audiodev.status=3<<0;
//
//						DMA_Cmd(DMA1_Stream5, ENABLE);
//						//DMA_Cmd(DMA1_Stream6, ENABLE);
//					}
					TIM_Cmd(TIM6, ENABLE);
					while(res==0)
					{
						while(wavtransferend==0);//等待wav传输完成;
						wavtransferend=0;
						if(fillnum!=WAV_DAC_TX_DMA_BUFSIZE)//播放结束?
						{
							//res=KEY0_PRES;
							break;
						}
//						if(wavctrl.nchannels == 1){
						if(wavdatabuf)fillnum=wav_buffill(audiodev.dacbuf2,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);//填充buf2
						else fillnum=wav_buffill(audiodev.dacbuf1,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);//填充buf1
//						}
//						else{
//							if(wavdatabuf)fillnum=wav_buffill(audiodev.dacbuf2,audiodev.dacbuf4,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);//填充buf2
//							else fillnum=wav_buffill(audiodev.dacbuf1,audiodev.dacbuf3,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);//填充buf1
//							fillnum = fillnum / 2;
//						}
						while(1)
						{
							key=KEY_Scan();
							if(key == 2)//暂停
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
							if(key == 1 ||key == 3)//下一曲/上一曲
							{
								res=key;
								break;
							}
							wav_get_curtime(audiodev.file,&wavctrl);//得到总时间和当前播放的时间
							//audio_msg_show(wavctrl.totsec,wavctrl.cursec,wavctrl.bitrate);
							t++;
//							if(t==20)
//							{
//								t=0;
//	 							LED0=!LED0;
//							}
							if((audiodev.status&0X01)==0){
								delay_ms(10);
							}
							else break;
						}
					}
					audiodev.status = 0;
					TIM_Cmd(TIM6, DISABLE);
					DMA_Cmd(DMA1_Stream5, DISABLE);
					DMA_Cmd(DMA1_Stream6, DISABLE);

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

void wavController(char* path){
	FRESULT res;
	FILINFO fno;
	DIR dir;
	u8 wavStatus;
	char *fn;

	audiodev.file=new FIL;
	audiodev.dacbuf1 = new u8[512];
	audiodev.dacbuf2 = new u8[512];
	audiodev.dacbuf3 = new u8[512];
	audiodev.dacbuf4 = new u8[512];
	audiodev.tbuf = new u8[1024];
	DAC_WAV_Init(audiodev.dacbuf1, audiodev.dacbuf2, audiodev.dacbuf3, audiodev.dacbuf4, 256);

	fno.lfsize = _MAX_LFN * 2 + 1;
	fno.lfname = new char[fno.lfsize];
	res = f_opendir(&dir,(const TCHAR*)path);
	if(res == FR_OK){
		res = f_readdir(&dir, &fno);
		if(res != FR_OK || fno.fname[0] == 0){
			return;
		}
		fn = *fno.lfname ? fno.lfname : fno.fname;
		//Init DAC, DMA, PA4
		//DAC_WAV_Init(audiodev.dacbuf1, audiodev.dacbuf2, 512);
		while(1){
			wavStatus = wav_play_song(append(fn));
			if(wavStatus != 0){
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
	delete [] audiodev.tbuf;
	delete [] audiodev.dacbuf1;
	delete [] audiodev.dacbuf2;
	delete [] audiodev.dacbuf3;
	delete [] audiodev.dacbuf4;
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
}

void DAC_WAV_Init(u8* bufCh1_0, u8* bufCh1_1, u8* bufCh2_0, u8* bufCh2_1, u16 num){
	//Channel 1 init
	DAC_WAV_Init(bufCh1_0, bufCh1_1, num);

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


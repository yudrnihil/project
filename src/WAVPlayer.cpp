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

/**
 * WAV Player
 * 2016.04.13 YU CHENDI
 * Play WAV file from FATFS SD Card.
 * Assume 8 bit mono file format at this moment.
 * Based on Alientek wav player and AN3126 application note
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
					wavx->bps=fmt->BitsPerSample;			//λ��,8/16/32

					wavx->datasize=data->ChunkSize;			//���ݿ��С
					wavx->datastart=wavx->datastart+8;		//��������ʼ�ĵط�.

					//init TIM6, set sample rate
					//connect TIM6 to DAC trigger
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
	delete ftemp;
	delete [] buf;
	return 0;
}

//���buf
//buf:������
//size:���������
//bits:λ��  8 at this moment
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
	if(audiodev.file&&audiodev.dacbuf1&&audiodev.dacbuf2&&audiodev.tbuf)
	{
		res=wav_decode_init(fname,&wavctrl);//�õ��ļ�����Ϣ
		if(res==0)//�����ļ��ɹ�
		{
			if(wavctrl.bps == 8)
			{
				TIM_Cmd(TIM6, ENABLE);
				DMA_Cmd(DMA1_Stream5, ENABLE);
				res=f_open(audiodev.file,(TCHAR*)fname,FA_READ);	//���ļ�
				if(res==0)
				{
					f_lseek(audiodev.file, wavctrl.datastart);		//�����ļ�ͷ
					fillnum=wav_buffill(audiodev.dacbuf1,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
					fillnum=wav_buffill(audiodev.dacbuf2,WAV_DAC_TX_DMA_BUFSIZE,wavctrl.bps);
					audiodev.status=3<<0;
					while(res==0)
					{
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
							key=KEY_Scan();
							if(key == 2)//��ͣ
							{
								if(audiodev.status&0X01)audiodev.status&=~(1<<0);
								else audiodev.status|=0X01;
							}
							if(key == 0||key == 3)//��һ��/��һ��
							{
								res=key;
								break;
							}
							wav_get_curtime(audiodev.file,&wavctrl);//�õ���ʱ��͵�ǰ���ŵ�ʱ��
							//audio_msg_show(wavctrl.totsec,wavctrl.cursec,wavctrl.bitrate);
							t++;
//							if(t==20)
//							{
//								t=0;
//	 							LED0=!LED0;
//							}
							if((audiodev.status&0X01)==0)delay_ms(10);
							else break;
						}
					}
					audiodev.status = 0;
					DMA_Cmd(DMA1_Stream5, DISABLE);
				}else res=0XFF; //err: cannot open file
			}else res=0XFF; //err: wav file is not 8bit, not supported yet
		}else res=0XFF; //err: cannot get file info
		delete [] audiodev.tbuf;
		delete [] audiodev.dacbuf1;
		delete [] audiodev.dacbuf2;
		delete audiodev.file;
	}
	else{
		res = 0xff; //err: cannot allocate memory
	}
	return res;
}

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
		DAC_WAV_Init(audiodev.dacbuf1, audiodev.dacbuf2, 512);
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
	//DMA_Cmd(DMA1_Stream5, ENABLE);

	//enable transfer complete interrupt
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;//��ռ���ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;//�����ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
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

/**
 * DMA1_Stream5_IRQHandler
 * called upon transfer completion
 * set the buffer currently in use
 */
void DMA1_Stream5_IRQHandler(void){
	if(DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5) == SET){
		DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5);
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
}


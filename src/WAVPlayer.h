/*
 * WAVPlayer.h
 *
 *  Created on: 2016��4��11��
 *      Author: YU Chendi
 */

#ifndef WAVPLAYER_H_
#define WAVPLAYER_H_
#include "stm32f4xx.h"
#include "fatfs/ff.h"

//size of DAC buffer
#define WAV_DAC_TX_DMA_BUFSIZE    512

//RIFF��
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"RIFF",��0X46464952
    u32 ChunkSize ;		   	//���ϴ�С;�ļ��ܴ�С-8
    u32 Format;	   			//��ʽ;WAVE,��0X45564157
}ChunkRIFF ;
//fmt��
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"fmt ",��0X20746D66
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:20.
    u16 AudioFormat;	  	//��Ƶ��ʽ;0X01,��ʾ����PCM;0X11��ʾIMA ADPCM
	u16 NumOfChannels;		//ͨ������;1,��ʾ������;2,��ʾ˫����;
	u32 SampleRate;			//������;0X1F40,��ʾ8Khz
	u32 ByteRate;			//�ֽ�����;
	u16 BlockAlign;			//�����(�ֽ�);
	u16 BitsPerSample;		//�����������ݴ�С;4λADPCM,����Ϊ4
//	u16 ByteExtraData;		//���ӵ������ֽ�;2��; ����PCM,û���������
}ChunkFMT;
//fact��
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"fact",��0X74636166;
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:4.
    u32 NumOfSamples;	  	//����������;
}ChunkFACT;
//LIST��
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"LIST",��0X74636166;
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:4.
}ChunkLIST;

//data��
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"data",��0X5453494C
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size)
}ChunkDATA;

//wavͷ
typedef __attribute__((__packed__)) struct
{
	ChunkRIFF riff;	//riff��
	ChunkFMT fmt;  	//fmt��
//	ChunkFACT fact;	//fact�� ����PCM,û������ṹ��
	ChunkDATA data;	//data��
}__WaveHeader;

//wav ���ſ��ƽṹ��
typedef __attribute__((__packed__)) struct
{
    u16 audioformat;			//��Ƶ��ʽ;0X01,��ʾ����PCM;0X11��ʾIMA ADPCM
	u16 nchannels;				//ͨ������;1,��ʾ������;2,��ʾ˫����;
	u16 blockalign;				//�����(�ֽ�);
	u32 datasize;				//WAV���ݴ�С

    u32 totsec ;				//���׸�ʱ��,��λ:��
    u32 cursec ;				//��ǰ����ʱ��

    u32 bitrate;	   			//������(λ��)
	u32 samplerate;				//������
	u16 bps;					//λ��,����16bit,24bit,32bit

	u32 datastart;				//����֡��ʼ��λ��(���ļ������ƫ��)
}__wavctrl;

//���ֲ��ſ�����
typedef __attribute__((__packed__)) struct
{
	u8* dacbuf1;
	u8* dacbuf2;
	u8 *tbuf;				//��ʱ����,����24bit�����ʱ����Ҫ�õ�
	FIL *file;				//��Ƶ�ļ�ָ��
	//0: stop, 3: play 2: pause
	u8 status;				//bit0:0,��ͣ����;1,��������
							//bit1:0,��������;1,��������
}__audiodev;

u8 wav_decode_init(char* fname,__wavctrl* wavx);
u32 wav_buffill(u8 *buf,u16 size,u8 bits);
void wav_i2s_dma_tx_callback(void);
u8 wav_play_song(char* fname);
void DAC_WAV_Init(u8* buf0, u8* buf1, u16 num);
void wavController(char* path);
#endif






















/*
 * WAVPlayer.h
 *
 *  Created on: 2016年4月11日
 *      Author: YU Chendi
 */

#ifndef WAVPLAYER_H_
#define WAVPLAYER_H_
#include "stm32f4xx.h"
#include "fatfs/ff.h"
#include "key.h"
//size of DAC buffer
#define WAV_DAC_TX_DMA_BUFSIZE    512

//RIFF
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;"RIFF",0X46464952
    u32 ChunkSize ;
    u32 Format;	   			//WAVE,0X45564157
}ChunkRIFF ;
//fmt
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;"fmt ",0X20746D66
    u32 ChunkSize ;		   	//20
    u16 AudioFormat;
	u16 NumOfChannels;		//1:mono 2:dual
	u32 SampleRate;
	u32 ByteRate;
	u16 BlockAlign;
	u16 BitsPerSample;
//	u16 ByteExtraData;
}ChunkFMT;
//fact
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;"fact",0X74636166;
    u32 ChunkSize ;		   	//4
    u32 NumOfSamples;	  	//
}ChunkFACT;
//LIST
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;"LIST",0X74636166;
    u32 ChunkSize ;		   	//4
}ChunkLIST;

//data
typedef __attribute__((__packed__)) struct
{
    u32 ChunkID;		   	//chunk id;"data",0X5453494C
    u32 ChunkSize ;
}ChunkDATA;

//wav头
typedef __attribute__((__packed__)) struct
{
	ChunkRIFF riff;	//riff
	ChunkFMT fmt;  	//fmt
//	ChunkFACT fact;	//fact
	ChunkDATA data;	//data
}__WaveHeader;

//wav control
typedef __attribute__((__packed__)) struct
{
    u16 audioformat;
	u16 nchannels;				//1: mono 2: dual
	u16 blockalign;
	u32 datasize;

    u32 bitrate;
	u32 samplerate;
	u16 bps;					//bit per sample

	u32 datastart;				//data offset
}__wavctrl;

//player control
typedef __attribute__((__packed__)) struct
{
	u8* dacbuf1;
	u8* dacbuf2;
	FIL *file;				//pointer to WAV file
	//0: stop, 3: play 2: pause
	u8 status;
}__audiodev;

u8 wav_decode_init(char* fname,__wavctrl* wavx);
u32 wav_buffill(u8 *buf,u16 size,u8 bits);
void wav_i2s_dma_tx_callback(void);
u8 wav_play_song(char* fname);
void DAC_WAV_Init(u8* buf0, u8* buf1, u16 num);
void DAC_WAV_Init(u8* bufCh1_0, u8* bufCh1_1, u8* bufCh2_0, u8* bufCh2_1, u16 num);
void wavController(char* path);
void DMA_Reconfigure(uint8_t channel);
u8 exf_getfree(char *drv,u32 *total,u32 *free);
u8 mf_scan_files(char* path);
#endif






















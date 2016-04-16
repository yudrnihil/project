/*
 * sound.cpp
 *
 *  Created on: 2016Äê4ÔÂ9ÈÕ
 *      Author: eric
 */

#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "sound.h"

uint16_t buffer[3][180],buf_note[3],decay[3];
uint16_t random[180]={41,2087,2239,1930,2789,3439,3288,693,2392,3989,1610,3575,2806,447,1771,491,2995,3752,732,1341,3726,2319,3902,153,292,97,1041,2336,3338,3515,1352,1251,2486,3348,1869,3532,1097,1729,655,1704,38,3336,2657,1668,1293,569,2856,3616,3683,2773,977,3074,3997,4092,3657,574,533,1551,2959,778,31,3035,1715,1842,288,1441,850,752,2884,2173,2876,3330,3605,2634,3895,3065,2721,2436,3918,3548,3249,338,3609,3574,2376,3650,871,3281,1646,1738,564,3774,56,3133,1442,1063,3833,2082,2454,161,738,2450,544,993,2229,1740,1692,2306,3008,1911,926,80,2354,2692,2175,1734,2207,3288,1002,132,3511,1005,971,2256,1880,197,3180,3289,4031,3862,2780,1150,561,1249,1681,3430,2442,1526,1627,3147,3172,2,3183,2193,2660,719,3544,1568,3746,2208,2327,376,2936,745,33,503,900,3926,2382,1655,1030,2264,3054,62,1073,2388,3025,4041,3602,3875,2101,2171,1184,2830,501,3546,2778,2724,3288,4009},buf_sum;
//uint16_t note_freq[8]={261,293,329,349,391,440,493},Sample_Rate=44100;
uint8_t  b=0,j=0,k=0,m=0;
int srt0=0,srt1=0,srt2=0;
uint16_t note_decay[9]={169,150,134,126,114,100,89,84,0};
int buf_inuse=1;

void Buf_Init(){
	for(int i=0;i<3;++i)
		for(int j=0;j<180;++j)
			buffer[i][j]=0;
}
//once get input
void Buf_Clear(int note){
    if(note<8)
	for(int n=0; n<note_decay[note];n++) buffer[buf_inuse][n]=random[n];
//	switch(buf_inuse){
//	case 0:b=0;srt0=0;break;
//	case 1:j=0;srt1=0;break;
//	case 2:k=0;srt2=0;break;
//	}
	buf_note[buf_inuse]=note;
	//(buf_inuse == 2)? buf_inuse = 0: buf_inuse++;

}

//calculate value for next interrupt
uint16_t Next_Round(){

	/*if(srt0)
        if(b!=note_decay[buf_note[0]]-1){
            	         buffer[0][b]=0.5 * (buffer[0][b]+buffer[0][b+1]);
            	       }
        else{
	buffer[0][b]=0.5 * (buffer[0][b]+buffer[0][0]);
	}*/

	if(srt1)
        if(j!=note_decay[buf_note[1]]-1){
            	         buffer[1][j]=0.5 * (buffer[1][j]+buffer[1][j+1]);
            	       }
        else{
	buffer[1][j]=0.5 * (buffer[1][j]+buffer[1][0]);
	}

	/*if(srt2)
        if(k!=note_decay[buf_note[2]]-1){
            	         buffer[2][k]=0.5 * (buffer[2][k]+buffer[2][k+1]);
            	       }
        else{
	buffer[2][k]=0.5 * (buffer[2][k]+buffer[2][0]);
	}*/


	buf_sum=buffer[1][j];//+buffer[2][k]+buffer[0][b];

//	if(b<note_decay[buf_note[0]]-1) b++; else {srt0=1;b=0;}
	if(j<note_decay[buf_note[1]]-1) j++; else {srt1=1;j=0;}
//	if(k<note_decay[buf_note[2]]-1) k++; else {srt2=1;k=0;}
	return buf_sum;}



/*
 * sound.cpp
 *
 *  Created on: 2016��4��9��
 *      Author: eric
 */

#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "sound.h"

uint16_t buffer[3][400],buf_note[3],decay[3];
uint16_t random[400]={42,2468,2335,2501,3170,3725,3479,1359,2963,465,1706,146,3282,828,1962,492,2996,3943,828,1437,392,2605,3903,154,293,383,1422,2717,3719,3896,1448,1727,2772,3539,1870,3913,1668,2300,1036,1895,704,3812,3323,2334,1674,665,3142,3712,254,2869,1548,3645,663,758,38,860,724,1742,3530,779,317,3036,2191,1843,289,2107,1041,943,3265,2649,3447,3806,3891,2730,371,3351,3007,3102,394,3549,3630,624,85,3955,2757,3841,967,3377,1932,2309,945,440,627,3324,1538,1539,119,2083,2930,542,834,3116,640,1659,2705,1931,1978,2307,3674,2387,1022,746,2925,3073,2271,1830,2778,3574,1098,513,3987,1291,1162,2637,2356,768,3656,3575,32,53,3351,1151,942,1725,1967,3431,3108,2192,2008,3338,3458,288,3754,2384,2946,910,210,1759,222,2589,2423,947,3507,1031,414,1169,901,592,2763,1656,1411,2360,3625,538,1549,2484,3596,42,3603,351,2292,2837,1375,3021,597,22,3349,3200,3669,485,282,735,54,2000,2419,3939,2901,3789,2128,468,3729,2894,649,2484,1808,2422,2311,2618,2814,1515,2310,3617,2936,1452,601,1250,520,3557,2799,2304,2225,3009,1845,610,2990,703,3196,486,3094,2344,2524,1588,1315,1504,3449,1201,1459,2619,581,3797,2799,3282,3590,799,10,3158,473,3623,2539,293,2039,180,2191,1658,3959,2192,3816,2889,3157,3512,203,2635,273,56,329,2647,2363,887,2876,434,1870,143,3845,1417,1882,3999,2323,2652,2022,1700,3558,477,3893,390,1076,2713,2601,2511,1004,2870,1862,2689,1402,1790,3256,424,1003,2586,183,2286,3089,3427,618,3758,1833,2933,170,2155,1722,1190,3977,3330,2369,693,1426,2556,3435,550,3442,1513,2146,2061,1719,3754,140,424,280,1997,688,530,2550,1438,3867,950,194,3196,3298,417,287,106,489,283,456,1735,2115,3702,3317,672,1787,264,314,356,3186,54,913,2809,1833,946,314,3757,322,3559,3647,3983,482,145,3197,223,3130,2162,1536,451,3174,2467,45,1660,2293,2440,1254,25,2155,1511,746,650,1187,314,475,23,2169,2019,2788,1906,1959,3392,2203,3626,2478,415,1315,1825,1335,1875,373,160,3834,71,3488,298,3519,178};
//uint16_t note_freq[8]={116,123,130,138,146,155,164,174,184,195,207,220,233,246,261,293,329,349,391,440,493,523,554,587},Sample_Rate=44100;
uint16_t  b=0,j=0,k=0,m=0;
int srt0=0,srt1=0,srt2=0;
uint16_t note_decay[24]={337,320,302,295,269,253,238,225,213,200,189,179,168,159,150,142,134,126,119,112,106,100,94,89};
int buf_sum = 0;
int buf_inuse=1;

void Buf_Init(){
	for(int i=0;i<3;++i)
		for(int j=0;j<400;++j)
			buffer[i][j]=2000;
}
//once get input
void Buf_Clear(int note){
    if(note<24)
	for(int n=0; n<note_decay[note];n++) buffer[buf_inuse][n]=random[n];
	switch(buf_inuse){
	case 0:b=0;srt0=0;break;
	case 1:j=0;srt1=0;break;
	case 2:k=0;srt2=0;break;
	}
	buf_note[buf_inuse]=note;
	(buf_inuse == 2)? buf_inuse = 0: buf_inuse++;

}

//calculate value for next interrupt
uint16_t Next_Round(){

	if(srt0)
        if(b!=note_decay[buf_note[0]]-1){
           buffer[0][b]=0.5 * (buffer[0][b]+buffer[0][b+1]);
            	       }
        else{
	       buffer[0][b]=0.5 * (buffer[0][b]+buffer[0][0]);
	    }
	if(srt1)
        if(j!=note_decay[buf_note[1]]-1){
            	         buffer[1][j]=0.5 * (buffer[1][j]+buffer[1][j+1]);
            	       }
        else{
	buffer[1][j]=0.5 * (buffer[1][j]+buffer[1][0]);
	}

	if(srt2)
        if(k!=note_decay[buf_note[2]]-1){
            	         buffer[2][k]=0.5 * (buffer[2][k]+buffer[2][k+1]);
            	       }
        else{
	buffer[2][k]=0.5 * (buffer[2][k]+buffer[2][0]);
	}


	buf_sum=buffer[1][j]+buffer[2][k]+buffer[0][b]-4000;
	if(buf_sum < 0 ) buf_sum=2000;

	if(b<note_decay[buf_note[0]]-1) b++; else {srt0=1;b=0;}
	if(j<note_decay[buf_note[1]]-1) j++; else {srt1=1;j=0;}
	if(k<note_decay[buf_note[2]]-1) k++; else {srt2=1;k=0;}
	return buf_sum;}



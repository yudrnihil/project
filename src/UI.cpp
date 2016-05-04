/*
 * UI.cpp
 *
 *  Created on: 2016Äê5ÔÂ3ÈÕ
 *      Author: YU Chendi
 */

#include "lcd.h"
#include "key.h"

uint8_t modeSelect(){
	//0: piano mode
	//1: wav mode
	uint8_t cur = 0;
	LCD_ShowString(30, 180, 200, 16, 16, "Piano mode");
	LCD_ShowString(30, 210, 200, 16, 16, "WAV player mode");
	if (cur == 0){
		LCD_Fill(250, 180, 270, 200, BLACK);
		LCD_Fill(250, 210, 270, 230, WHITE);
	}
	else{
		LCD_Fill(250, 180, 270, 200, WHITE);
		LCD_Fill(250, 210, 270, 230, BLACK);
	}
	while(1){
		uint8_t key = KEY_Scan();
		if (cur == 0){
			LCD_Fill(250, 180, 270, 200, BLACK);
			LCD_Fill(250, 210, 270, 230, WHITE);
		}
		else{
			LCD_Fill(250, 180, 270, 200, WHITE);
			LCD_Fill(250, 210, 270, 230, BLACK);
		}
		switch(key){
			case 0:
				break;
			case 3:
				break;
			case 4:
			case 2:
				cur = 1 - cur;
				break;
			case 1:
				LCD_Fill(30, 180, 270, 230, WHITE);
				return cur;
		}
	}
}



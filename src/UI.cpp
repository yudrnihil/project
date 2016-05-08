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

		LCD_Fill(250, 180, 270, 200, BLACK);
		LCD_Fill(250, 210, 270, 230, WHITE);

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

uint8_t timbreSelect(){
	//0: piano
	//1: nylon
	//2: stone
	int8_t cur = 0;
	LCD_ShowString(30, 180, 200, 16, 16, "Piano");
	LCD_ShowString(30, 210, 200, 16, 16, "Stone");
	LCD_ShowString(30, 240, 200, 16, 16, "Space");

		LCD_Fill(250, 180, 270, 200, BLACK);
		LCD_Fill(250, 210, 270, 230, WHITE);
		LCD_Fill(250, 240, 270, 260, WHITE);

	while(1){
		uint8_t key = KEY_Scan();
		if (cur == 0){
			LCD_Fill(250, 180, 270, 200, BLACK);
			LCD_Fill(250, 210, 270, 230, WHITE);
			LCD_Fill(250, 240, 270, 260, WHITE);
		}
		else if (cur == 1){
			LCD_Fill(250, 180, 270, 200, WHITE);
			LCD_Fill(250, 210, 270, 230, BLACK);
			LCD_Fill(250, 240, 270, 260, WHITE);
		}
		else{
			LCD_Fill(250, 180, 270, 200, WHITE);
			LCD_Fill(250, 210, 270, 230, WHITE);
			LCD_Fill(250, 240, 270, 260, BLACK);
		}
		switch(key){
			case 0:
				break;
			case 3:
				break;
			case 4:
				if(cur > 0){
				cur--;
				}
				break;
			case 2:
				if(cur < 2){
				cur++;
				}
				break;
			case 1:
				LCD_Fill(30, 180, 270, 260, WHITE);
				return cur;
		}
	}
}



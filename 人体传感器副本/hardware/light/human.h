#ifndef __HUMAN_H
#define __HUMAN_H

#include "stm32f10x.h"

void Human_Init(void);
uint8_t Human_Detect(void);
void Human_LED_Control(uint8_t human_status);
void Human_Update_Display(uint8_t human_status);

#endif

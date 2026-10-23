#ifndef __LIGHT_H
#define __LIGHT_H

#include "stm32f10x.h"

void Light_Init(void);
uint16_t Light_GetAdc(void);
uint16_t Light_GetAdcAverage(void);
uint16_t Light_GetLux(void);   // 返回农业场景光照强度，单位 Lux，范围 0~50000
void Delay_ms(uint16_t ms);

#endif

#include "rain.h"

void  Rain_Init(void)
{ 	
 GPIO_InitTypeDef GPIO_InitStructure;
 
 /* Enable GPIOA clock */
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
 
 /* Configure PA.04 pin as input floating */
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
 GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void  GetValue_Rain(u8 *rain)
{ 	
	uint8_t ret;
		// 获取引脚状态
	ret = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4);
	if(ret == Bit_RESET)		
	{
		*rain = 1;			//低电平，有雨雪
	}
	else
	{
		*rain = 0;			//高电平，无雨雪
	}
}


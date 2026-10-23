#include "lock.h"
#include "delay.h"

void Lock_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//??GPIOB??
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//????
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//IO????50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure); //???GPIOE.3~5
	GPIO_ResetBits(GPIOB, GPIO_Pin_6 );//?????
}

void Control_Lock(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_6 );
	delay_ms(2000);
	GPIO_ResetBits(GPIOB, GPIO_Pin_6 );//?????
}


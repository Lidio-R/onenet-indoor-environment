#include "led.h"

void LED_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_APB2PeriphClockCmd(LED_RCC, ENABLE);//使能GPIOE时钟
  GPIO_InitStructure.GPIO_Pin = LED_PIN ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//IO口速度为50MHz
  GPIO_Init(LED_PORT, &GPIO_InitStructure); //初始化GPIOE.3~5
  GPIO_SetBits(LED_PORT, LED_PIN );//初始化led灭
}

void Control_LED(u8 cmd)
{
	if(cmd == 1)
	{
		GPIO_ResetBits(LED_PORT,LED_PIN); //点亮LED
	}
	else{
		GPIO_SetBits(LED_PORT, LED_PIN);  //熄灭LED
	}
}


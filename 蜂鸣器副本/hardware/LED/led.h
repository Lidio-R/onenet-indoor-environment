#include "stm32f10x.h"

#define  	LED_RCC		RCC_APB2Periph_GPIOB
#define		LED_PORT	GPIOB
#define		LED_PIN		GPIO_Pin_1


void LED_Init(void);
void Control_LED(u8 cmd);



#include "stm32f10x.h"

#define  	S12BUZZER_RCC		RCC_APB2Periph_GPIOA
#define		S12BUZZER_PORT	GPIOA
#define		S12BUZZER_PIN		GPIO_Pin_6


void Buzzer_Init(void);
void Control_Buzzer(u8 cmd);



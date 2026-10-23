#include "stm32f10x.h"

/* LED Definitions - Only Green LED on PB1 */
#define  	LED_GREEN_RCC	RCC_APB2Periph_GPIOB
#define		LED_GREEN_PORT	GPIOB
#define		LED_GREEN_PIN	GPIO_Pin_1

void LED_Init(void);
void LED_Green_On(void);
void LED_Green_Off(void);
void LED_All_Off(void);

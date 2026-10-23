/**
 * @file led.h
 * @brief LED Driver Header - PB1 Green LED only
 */

#ifndef _LED_H_
#define _LED_H_

#include "stm32f10x.h"

/* Green LED - PB1 */
#define LED_GREEN_RCC   RCC_APB2Periph_GPIOB
#define LED_GREEN_PORT  GPIOB
#define LED_GREEN_PIN   GPIO_Pin_1

void LED_Init(void);
void LED_Green_On(void);
void LED_Green_Off(void);

#endif

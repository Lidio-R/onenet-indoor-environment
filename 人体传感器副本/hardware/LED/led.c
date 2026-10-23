#include "led.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable Green LED clock */
    RCC_APB2PeriphClockCmd(LED_GREEN_RCC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = LED_GREEN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GREEN_PORT, &GPIO_InitStructure);
    GPIO_SetBits(LED_GREEN_PORT, LED_GREEN_PIN);  /* Turn off initially */
}

void LED_Green_On(void)
{
    GPIO_ResetBits(LED_GREEN_PORT, LED_GREEN_PIN);
}

void LED_Green_Off(void)
{
    GPIO_SetBits(LED_GREEN_PORT, LED_GREEN_PIN);
}

void LED_All_Off(void)
{
    GPIO_SetBits(LED_GREEN_PORT, LED_GREEN_PIN);
}

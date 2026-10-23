#include "light.h"
#include "stm32f10x.h"

#define HUMAN_GPIO_PORT    GPIOA
#define HUMAN_GPIO_PIN     GPIO_Pin_4
#define HUMAN_GPIO_RCC     RCC_APB2Periph_GPIOA

void Human_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(HUMAN_GPIO_RCC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = HUMAN_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(HUMAN_GPIO_PORT, &GPIO_InitStructure);
}

uint8_t Human_Detect(void)
{
    return GPIO_ReadInputDataBit(HUMAN_GPIO_PORT, HUMAN_GPIO_PIN);
}
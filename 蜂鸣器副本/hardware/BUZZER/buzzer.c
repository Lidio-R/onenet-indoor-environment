#include "buzzer.h"
#include "delay.h"
void Buzzer_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(S12BUZZER_RCC, ENABLE);
    GPIO_ResetBits(S12BUZZER_PORT, S12BUZZER_PIN);
    GPIO_InitStructure.GPIO_Pin = S12BUZZER_PIN ;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(S12BUZZER_PORT, &GPIO_InitStructure);

    GPIO_ResetBits(S12BUZZER_PORT, S12BUZZER_PIN);
}

void Control_Buzzer(u8 cmd)
{
    if (cmd == 1)
    {
        GPIO_SetBits(S12BUZZER_PORT, S12BUZZER_PIN);
    }
    else
    {
        GPIO_ResetBits(S12BUZZER_PORT, S12BUZZER_PIN);
    }


}

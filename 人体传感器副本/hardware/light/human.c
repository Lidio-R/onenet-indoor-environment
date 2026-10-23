#include "human.h"
#include "stm32f10x.h"
#include "led.h"
#include "oled.h"

#define HUMAN_GPIO_PORT    GPIOA
#define HUMAN_GPIO_PIN     GPIO_Pin_4
#define HUMAN_GPIO_RCC     RCC_APB2Periph_GPIOA

/**
 * @brief Initialize human sensor GPIO
 */
void Human_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(HUMAN_GPIO_RCC, ENABLE);

    // Configure GPIO as input with pull-up resistor
    // HC-SR501 outputs high when no motion, low when motion detected
    GPIO_InitStructure.GPIO_Pin = HUMAN_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // Input with pull-up
    GPIO_Init(HUMAN_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief Detect human presence
 * @return 0=Human detected (motion), 1=No human (no motion)
 */
uint8_t Human_Detect(void)
{
    return GPIO_ReadInputDataBit(HUMAN_GPIO_PORT, HUMAN_GPIO_PIN);
}

/**
 * @brief Control LED based on human detection
 * @param human_status: Human detection status (0=Human detected, 1=No human)
 */
void Human_LED_Control(uint8_t human_status)
{
    if(human_status == 0)  // Human detected (low level)
    {
        /* Human detected - Green LED on */
        LED_Green_On();
    }
    else  // No human (high level)
    {
        /* No human - All LEDs off */
        LED_All_Off();
    }
}

/**
 * @brief Update OLED display with human sensor status
 * @param human_status: Human detection status (0=Human detected, 1=No human)
 */
void Human_Update_Display(uint8_t human_status)
{
    OLED_ShowString(0, 0, "Human: ", 12);
    OLED_ShowNum(48, 0, human_status, 1, 12);
    OLED_ShowString(56, 0, (u8*)(human_status ? "No" : "Detected"), 12);
}

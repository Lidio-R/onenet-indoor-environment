/**
 * @file main.c
 * @brief STM32F103 OneNET MQTT — WeChat Mini-Program LED Control + Auto Light Sensing
 * 
 * PB1 Green LED control:
 * - Manual mode: mini-program sends ctl_led via OneNET service invoke
 * - Auto mode:   TIM2 interrupt (100ms) controls LED by light threshold
 */

#include "stm32f10x.h"
#include "onenet.h"
#include "esp8266.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include <string.h>
#include "light.h"
#include "led.h"

#define LIGHT_THRESHOLD   1000

u16 light = 0;
volatile u8 led_mode = 0;
volatile u8 manual_led_state = 0;

void TIM2_Light_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    TIM_TimeBaseStructure.TIM_Period = 999;
    TIM_TimeBaseStructure.TIM_Prescaler = 7199;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        
        if(led_mode == 0)
        {
            if(light < LIGHT_THRESHOLD)
                LED_Green_On();
            else
                LED_Green_Off();
        }
    }
}

void Read_Sensors(void)
{
    light = Light_GetLux();
}

int main(void)
{
    unsigned char *dataPtr = NULL;
    unsigned int send_count = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart3_Init(115200);
    Usart2_Init(115200);
    ESP8266_Init();
    LED_Init();
    OLED_Init();
    Light_Init();
    TIM2_Light_Init();
    
    LED_Green_Off();
    
    UsartPrintf(USART_DEBUG, "Hardware init OK - PB1 Green LED, TIM2 IRQ ready\r\n");
    
    OLED_Clear();
    OLED_ShowString(0, 0, "OneNet LED Ctrl", 16);

    UsartPrintf(USART_DEBUG, "Connect MQTTs Server...\r\n");
    while(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))
        delay_ms(500);
    UsartPrintf(USART_DEBUG, "NET_OK\r\n");
    
    UsartPrintf(USART_DEBUG, "Connecting to OneNET...\r\n");
    while(OneNet_DevLink())
        delay_ms(500);
    UsartPrintf(USART_DEBUG, "OneNET Connected!\r\n");
    
    UsartPrintf(USART_DEBUG, "Subscribing to LED control service...\r\n");
    OneNET_Subscribe();
    UsartPrintf(USART_DEBUG, "Subscribe OK!\r\n");

    while(1)
    {
        Read_Sensors();
        
        OLED_ShowString(0, 0, "Light: ", 12);
        OLED_ShowNum(48, 0, light, 5, 12);
        OLED_ShowString(88, 0, "Lux", 12);
        
        if(led_mode == 0)
            OLED_ShowString(0, 2, "Mode: AUTO ", 12);
        else
            OLED_ShowString(0, 2, "Mode: MANUAL", 12);
        
        dataPtr = ESP8266_GetIPD(0);
        if(dataPtr != NULL)
            OneNet_RevPro(dataPtr);
        
        send_count++;
        if(send_count >= 10)
        {
            send_count = 0;
            OneNet_SendData();
            OneNET_PushLightToFan();
            UsartPrintf(USART_DEBUG, "Data Uploaded: Light=%d Lux, Mode=%s\r\n", 
                       light, led_mode ? "MANUAL" : "AUTO");
        }
        
        delay_ms(100);
    }
}
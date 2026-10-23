/**
 * @file main.c
 * @brief STM32F103 OneNET MQTT Human Sensor Data Upload Example
 * 
 * This program connects to OneNET IoT platform via MQTT protocol
 * and uploads human sensor data.
 * 
 * Sensor Mapping:
 * - Pir: Human detection (bool, true=Human detected, false=No human)
 * 
 * LED Control:
 * - Human detected: Green LED on
 * - No human: LED off
 */

#include "stm32f10x.h"
#include "onenet.h"
#include "esp8266.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include <string.h>
#include "human.h"
#include "led.h"

/* Sensor data variables */
u8 human = 0;           /* Human detection status (0=Human detected, 1=No human) */

/**
 * @brief Main function
 * @return None
 */
int main(void)
{
    unsigned char *dataPtr = NULL;  /* Pointer for received data from ESP8266 */
    unsigned int send_count = 0;    /* Counter for periodic data upload */

    /* System initialization */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  /* Configure NVIC priority group */
    Delay_Init();                                    /* Initialize SysTick timer */
    Usart3_Init(115200);                            /* Initialize USART3 for debug output */
    Usart2_Init(115200);                            /* Initialize USART2 for ESP8266 */
    ESP8266_Init();                                 /* Initialize ESP8266 WiFi module */
    LED_Init();                                     /* Initialize LED indicators */
    OLED_Init();                                    /* Initialize OLED display */
    Human_Init();                                   /* Initialize human sensor */
    
    /* Turn off all LEDs initially */
    LED_All_Off();
    
    UsartPrintf(USART_DEBUG, "Hardware init OK\r\n");
    
    /* Clear OLED and display welcome message */
    OLED_Clear();
    OLED_ShowString(0, 0, "OneNet Human", 16);

    /* Connect to MQTT server */
    UsartPrintf(USART_DEBUG, "Connect MQTTs Server...\r\n");
    while(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))
        delay_ms(500);
    UsartPrintf(USART_DEBUG, "NET_OK\r\n");
    
    /* Connect to OneNET platform */
    UsartPrintf(USART_DEBUG, "Connecting to OneNET...\r\n");
    while(OneNet_DevLink())
        delay_ms(500);
    UsartPrintf(USART_DEBUG, "OneNET Connected!\r\n");

    /* Main application loop */
    while(1)
    {
        /* Read human sensor data */
        human = Human_Detect();
        
        /* Control LEDs based on human detection */
        Human_LED_Control(human);
        
        /* Update OLED display */
        Human_Update_Display(human);
        
        /* Process downlink data from cloud platform */
        dataPtr = ESP8266_GetIPD(0);
        if(dataPtr != NULL)
            OneNet_RevPro(dataPtr);
        
        /* Upload data every 1 second (10 * 100ms) */
        send_count++;
        if(send_count >= 10)
        {
            send_count = 0;
            OneNet_SendData();
            UsartPrintf(USART_DEBUG, "Data Uploaded: Pir=%s\r\n", human ? "false" : "true");
        }
        
        delay_ms(100);
    }
}

#include "stm32f10x.h"
#include "onenet.h"
#include "esp8266.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include <string.h>
#include "led.h"
#include "Sensor.h"
#include "buzzer.h"

uint16_t smoke = 0;

extern uint8_t g_buzzer_enabled;

int main(void)
{

    unsigned short timeCount = 0;	//发送间隔变量

    unsigned char *dataPtr = NULL;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//中断控制器分组设置
    Delay_Init();									//systick初始化
    Usart3_Init(115200);							//串口3，打印信息用
    Usart2_Init(115200);							//串口2，驱动ESP8266用
    ESP8266_Init();					//初始化ESP8266
    OLED_Init();
    LED_Init();
    UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
    Sensor_Init();
    Buzzer_Init();
    OLED_Clear(); 																			//清屏
    UsartPrintf(USART_DEBUG, "Connect MQTTs Server...\r\n");
    while (ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT"))
        delay_ms(500);
    UsartPrintf(USART_DEBUG, "NET_OK\r\n");

    while (OneNet_DevLink())			//接入OneNET
        delay_ms(500);
		
    OneNET_Subscribe();
    while (1)
    {
        static uint16_t buzzer_tick = 0;
        static uint16_t send_count = 0;

        // 处理云平台下行
        dataPtr = ESP8266_GetIPD(0);
        if (dataPtr != NULL)
            OneNet_RevPro(dataPtr);

        // 蜂鸣器间歇控制: 响1秒(10 ticks), 停2秒(20 ticks), 循环
        if (g_buzzer_enabled) {
            buzzer_tick++;
            if (buzzer_tick <= 10) {
                Control_Buzzer(1);   // 0~1秒: 蜂鸣器响
            } else if (buzzer_tick <= 30) {
                Control_Buzzer(0);   // 1~3秒: 蜂鸣器停
            } else {
                buzzer_tick = 0;      // 重置周期
            }
        } else {
            Control_Buzzer(0);
            buzzer_tick = 0;
        }

        // 每 3 秒(30 ticks) 上报一次设备状态到 OneNET 平台
        send_count++;
        if (send_count >= 30) {
            send_count = 0;
            OneNet_SendData();
            UsartPrintf(USART_DEBUG, "Status Uploaded: buzzer_switch=%s\r\n",
                       g_buzzer_enabled ? "true" : "false");
        }

        delay_ms(100);
    }


}

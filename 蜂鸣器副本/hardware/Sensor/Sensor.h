#ifndef __SENSOR_H
#define __SENSOR_H

#include "stm32f10x.h"
#include "delay.h"

// 可燃气体传感器 → 接在 PA7
#define		S10COMBUSTIBLEGAS_RCC        RCC_APB2Periph_GPIOA
#define		S10COMBUSTIBLEGAS_PORT       GPIOA
#define		S10COMBUSTIBLEGAS_PIN        GPIO_Pin_7
#define		S10COMBUSTIBLEGAS_ADC_NCH    1    // 转换通道数量 = 1
#define		S10COMBUSTIBLEGAS_ADC_CH     ADC_Channel_7

// ADC 配置
#define		SENSOR_ADC_RCC               RCC_APB2Periph_ADC1
#define		SENSOR_ADC                   ADC1

void  Sensor_Init(void);
uint16_t Get_Adc(void);
uint16_t Get_Adc_Average(void);

#endif

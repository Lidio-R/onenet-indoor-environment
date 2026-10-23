#include "sensor.h"
#include "stm32f10x.h"
#include "stdio.h"
#include "delay.h"

void Sensor_Init(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(S10COMBUSTIBLEGAS_RCC | SENSOR_ADC_RCC, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	GPIO_InitStructure.GPIO_Pin = S10COMBUSTIBLEGAS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(S10COMBUSTIBLEGAS_PORT, &GPIO_InitStructure);

	ADC_DeInit(SENSOR_ADC);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(SENSOR_ADC, &ADC_InitStructure);

	ADC_Cmd(SENSOR_ADC, ENABLE);
	ADC_ResetCalibration(SENSOR_ADC);
	while(ADC_GetResetCalibrationStatus(SENSOR_ADC));
	ADC_StartCalibration(SENSOR_ADC);
	while(ADC_GetCalibrationStatus(SENSOR_ADC));
}

uint16_t Get_Adc(void)
{
	// 这里统一！！！和初始化通道一致
	ADC_RegularChannelConfig(SENSOR_ADC, S10COMBUSTIBLEGAS_ADC_CH, 1, ADC_SampleTime_239Cycles5);
	ADC_SoftwareStartConvCmd(SENSOR_ADC, ENABLE);
	while(!ADC_GetFlagStatus(SENSOR_ADC, ADC_FLAG_EOC));
	return ADC_GetConversionValue(SENSOR_ADC);
}

uint16_t Get_Adc_Average(void)
{
	uint32_t temp=0;
	uint8_t t;
	for(t=0;t<10;t++)
	{
		temp += Get_Adc();
		delay_ms(2);
	}
	// 把平均值放大，让变化更明显
	return (temp/10) * 2; 
}


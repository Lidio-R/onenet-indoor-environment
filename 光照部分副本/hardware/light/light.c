#include "light.h"
#include "stm32f10x.h"

/* Light sensor configuration */
#define LIGHT_GPIO_PORT    GPIOA
#define LIGHT_GPIO_PIN     GPIO_Pin_7
#define LIGHT_GPIO_RCC     RCC_APB2Periph_GPIOA
#define LIGHT_ADC          ADC1
#define LIGHT_ADC_RCC      RCC_APB2Periph_ADC1
#define LIGHT_ADC_CHANNEL  ADC_Channel_7

/* Calibration parameters */
#define LIGHT_CAL_ADC1     1200    /* ADC value at 300 Lux */
#define LIGHT_CAL_LUX1     300     /* Calibration point 1 (Lux) */
#define LIGHT_CAL_ADC2     3000    /* ADC value at 8000 Lux */
#define LIGHT_CAL_LUX2     8000    /* Calibration point 2 (Lux) */

/**
 * @brief Delay function for sensor sampling
 */
static void Delay_Sensor(u16 x)
{
    u32 a, b;
    for(a = x; a > 0; a--)
    {
        for(b = 0x0001FFFF; b > 0; b--);
    }
}

/**
 * @brief Initialize light sensor ADC
 */
void Light_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIO and ADC clock */
    RCC_APB2PeriphClockCmd(LIGHT_GPIO_RCC | LIGHT_ADC_RCC, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* Configure GPIO as analog input */
    GPIO_InitStructure.GPIO_Pin = LIGHT_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(LIGHT_GPIO_PORT, &GPIO_InitStructure);

    /* ADC configuration */
    ADC_DeInit(LIGHT_ADC);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(LIGHT_ADC, &ADC_InitStructure);

    /* Enable ADC and calibration */
    ADC_Cmd(LIGHT_ADC, ENABLE);
    ADC_ResetCalibration(LIGHT_ADC);
    while(ADC_GetResetCalibrationStatus(LIGHT_ADC));
    ADC_StartCalibration(LIGHT_ADC);
    while(ADC_GetCalibrationStatus(LIGHT_ADC));
}

/**
 * @brief Read ADC value from light sensor
 */
uint16_t Light_GetAdc(void)
{
    ADC_RegularChannelConfig(LIGHT_ADC, LIGHT_ADC_CHANNEL, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(LIGHT_ADC, ENABLE);
    while(!ADC_GetFlagStatus(LIGHT_ADC, ADC_FLAG_EOC));
    return ADC_GetConversionValue(LIGHT_ADC);
}

/**
 * @brief Read average ADC value (10 samples)
 */
uint16_t Light_GetAdcAverage(void)
{
    uint32_t temp = 0;
    uint8_t t;

    for(t = 0; t < 10; t++)
    {
        temp += Light_GetAdc();
        Delay_Sensor(2);
    }
    return (uint16_t)(temp / 10);
}

/**
 * @brief Get light intensity in Lux
 * @return Light intensity (0~50000 Lux)
 */
uint16_t Light_GetLux(void)
{
    uint16_t adc = Light_GetAdcAverage();
    float lux;

    /* Linear interpolation */
    lux = (float)(adc - LIGHT_CAL_ADC1) * (LIGHT_CAL_LUX2 - LIGHT_CAL_LUX1)
          / (LIGHT_CAL_ADC2 - LIGHT_CAL_ADC1) + LIGHT_CAL_LUX1;

    /* Clamp to valid range */
    if(lux < 0) lux = 0;
    if(lux > 50000) lux = 50000;

    return (uint16_t)lux;
}
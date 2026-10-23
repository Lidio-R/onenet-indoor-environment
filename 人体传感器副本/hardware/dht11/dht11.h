/**
 * @file dht11.h
 * @brief DHT11 Temperature and Humidity Sensor Driver Header
 */

#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/* DHT11 GPIO Configuration */
#define DHT11_GPIO_PORT    GPIOB
#define DHT11_GPIO_PIN     GPIO_Pin_12
#define DHT11_GPIO_RCC     RCC_APB2Periph_GPIOB

/* Function prototypes */
void DHT11_Init(void);
uint8_t DHT11_ReadData(float *temperature, float *humidity);

#endif /* __DHT11_H */

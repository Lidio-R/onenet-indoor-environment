/**
 * @file sht20.h
 * @brief SHT20 Temperature and Humidity Sensor Driver Header
 * 
 * SHT20 is an I2C interface digital temperature and humidity sensor.
 * I2C Address: 0x40 (7-bit)
 */

#ifndef __SHT20_H
#define __SHT20_H

#include "stm32f10x.h"

/* SHT20 I2C Configuration */
#define SHT20_I2C           I2C1
#define SHT20_I2C_RCC       RCC_APB1Periph_I2C1
#define SHT20_SCL_PORT      GPIOB
#define SHT20_SCL_PIN       GPIO_Pin_6
#define SHT20_SCL_RCC       RCC_APB2Periph_GPIOB
#define SHT20_SDA_PORT      GPIOB
#define SHT20_SDA_PIN       GPIO_Pin_7
#define SHT20_SDA_RCC       RCC_APB2Periph_GPIOB

/* SHT20 I2C Address */
#define SHT20_ADDR          0x40

/* SHT20 Commands */
#define SHT20_CMD_TEMP_HOLD    0xE3    /* Measure temperature, hold master */
#define SHT20_CMD_HUMI_HOLD    0xE5    /* Measure humidity, hold master */
#define SHT20_CMD_TEMP_NOHOLD  0xF3    /* Measure temperature, no hold */
#define SHT20_CMD_HUMI_NOHOLD  0xF5    /* Measure humidity, no hold */
#define SHT20_CMD_WRITE_REG    0xE6    /* Write user register */
#define SHT20_CMD_READ_REG     0xE7    /* Read user register */
#define SHT20_CMD_SOFT_RESET   0xFE    /* Soft reset */

/* Function prototypes */
void SHT20_Init(void);
uint8_t SHT20_ReadData(float *temperature, float *humidity);

#endif /* __SHT20_H */
/**
 * @file sht20.c
 * @brief SHT20 Temperature and Humidity Sensor Driver Implementation
 */

#include "sht20.h"
#include "delay.h"

/**
 * @brief Initialize I2C for SHT20
 */
void SHT20_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    
    /* Enable I2C and GPIO clocks */
    RCC_APB1PeriphClockCmd(SHT20_I2C_RCC, ENABLE);
    RCC_APB2PeriphClockCmd(SHT20_SCL_RCC | SHT20_SDA_RCC, ENABLE);
    
    /* Configure SCL pin */
    GPIO_InitStructure.GPIO_Pin = SHT20_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SHT20_SCL_PORT, &GPIO_InitStructure);
    
    /* Configure SDA pin */
    GPIO_InitStructure.GPIO_Pin = SHT20_SDA_PIN;
    GPIO_Init(SHT20_SDA_PORT, &GPIO_InitStructure);
    
    /* Configure I2C */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 400000;  /* 400kHz */
    
    I2C_Init(SHT20_I2C, &I2C_InitStructure);
    I2C_Cmd(SHT20_I2C, ENABLE);
    
    /* Soft reset */
    I2C_GenerateSTART(SHT20_I2C, ENABLE);
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_MODE_SELECT));
    
    I2C_Send7bitAddress(SHT20_I2C, SHT20_ADDR << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    
    I2C_SendData(SHT20_I2C, SHT20_CMD_SOFT_RESET);
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    
    I2C_GenerateSTOP(SHT20_I2C, ENABLE);
    
    delay_ms(15);  /* Wait for reset to complete */
}

/**
 * @brief Read raw data from SHT20
 * @param cmd Measurement command
 * @param data Pointer to store raw data
 * @return 0 - Success, 1 - Error
 */
static uint8_t SHT20_ReadRaw(uint8_t cmd, uint16_t *data)
{
    /* Wait for previous measurement to complete */
    delay_ms(80);
    
    /* Send start condition */
    I2C_GenerateSTART(SHT20_I2C, ENABLE);
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_MODE_SELECT));
    
    /* Send device address with write */
    I2C_Send7bitAddress(SHT20_I2C, SHT20_ADDR << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    
    /* Send command */
    I2C_SendData(SHT20_I2C, cmd);
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    
    /* Send start condition again (repeated start) */
    I2C_GenerateSTART(SHT20_I2C, ENABLE);
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_MODE_SELECT));
    
    /* Send device address with read */
    I2C_Send7bitAddress(SHT20_I2C, SHT20_ADDR << 1, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
    
    /* Read first byte (MSB) */
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED));
    *data = (uint16_t)I2C_ReceiveData(SHT20_I2C) << 8;
    
    /* Enable acknowledge */
    I2C_AcknowledgeConfig(SHT20_I2C, ENABLE);
    
    /* Read second byte (LSB) */
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED));
    *data |= I2C_ReceiveData(SHT20_I2C);
    
    /* Disable acknowledge for last byte */
    I2C_AcknowledgeConfig(SHT20_I2C, DISABLE);
    
    /* Read checksum (not used here) */
    while(!I2C_CheckEvent(SHT20_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED));
    I2C_ReceiveData(SHT20_I2C);
    
    /* Send stop condition */
    I2C_GenerateSTOP(SHT20_I2C, ENABLE);
    
    return 0;
}

/**
 * @brief Read temperature and humidity from SHT20
 * @param temperature Pointer to store temperature value (°C)
 * @param humidity Pointer to store humidity value (%RH)
 * @return 0 - Success, 1 - Error
 */
uint8_t SHT20_ReadData(float *temperature, float *humidity)
{
    uint16_t raw_temp, raw_humi;
    
    /* Read humidity */
    if(SHT20_ReadRaw(SHT20_CMD_HUMI_HOLD, &raw_humi) != 0)
        return 1;
    
    /* Read temperature */
    if(SHT20_ReadRaw(SHT20_CMD_TEMP_HOLD, &raw_temp) != 0)
        return 1;
    
    /* Convert raw values to actual values */
    *humidity = (-6.0f) + (125.0f / 65536.0f) * (float)(raw_humi & 0xFFFC);
    *temperature = (-46.85f) + (175.72f / 65536.0f) * (float)(raw_temp & 0xFFFC);
    
    return 0;
}
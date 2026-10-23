/**
 * @file dht11.c
 * @brief DHT11 Temperature and Humidity Sensor Driver Implementation
 */

#include "dht11.h"
#include "delay.h"

/**
 * @brief Set DHT11 pin as output
 */
static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief Set DHT11 pin as input
 */
static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief Send start signal to DHT11
 */
static void DHT11_SendStart(void)
{
    DHT11_SetOutput();
    GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
    Delay_ms(18);
    GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
    Delay_us(30);
    DHT11_SetInput();
}

/**
 * @brief Wait for DHT11 response
 * @return 0 - Response received, 1 - No response
 */
static uint8_t DHT11_WaitResponse(void)
{
    uint16_t timeout = 0;
    
    while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == SET)
    {
        timeout++;
        Delay_us(1);
        if(timeout > 100) return 1;
    }
    
    timeout = 0;
    while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == RESET)
    {
        timeout++;
        Delay_us(1);
        if(timeout > 100) return 1;
    }
    
    timeout = 0;
    while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == SET)
    {
        timeout++;
        Delay_us(1);
        if(timeout > 100) return 1;
    }
    
    return 0;
}

/**
 * @brief Read one byte from DHT11
 * @return Read byte value
 */
static uint8_t DHT11_ReadByte(void)
{
    uint8_t data = 0;
    uint8_t i;
    
    for(i = 0; i < 8; i++)
    {
        while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == RESET);
        
        Delay_us(30);
        
        data <<= 1;
        if(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == SET)
        {
            data |= 0x01;
        }
        
        while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == SET);
    }
    
    return data;
}

/**
 * @brief Initialize DHT11 sensor
 */
void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(DHT11_GPIO_RCC, ENABLE);
    
    DHT11_SetOutput();
    GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
    Delay_ms(100);
}

/**
 * @brief Read temperature and humidity from DHT11
 * @param temperature Pointer to store temperature value
 * @param humidity Pointer to store humidity value
 * @return 0 - Success, 1 - Error
 */
uint8_t DHT11_ReadData(float *temperature, float *humidity)
{
    uint8_t buf[5];
    uint8_t i;
    
    DHT11_SendStart();
    
    if(DHT11_WaitResponse() != 0)
    {
        return 1;
    }
    
    for(i = 0; i < 5; i++)
    {
        buf[i] = DHT11_ReadByte();
    }
    
    DHT11_SetOutput();
    GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
    
    /* Check checksum */
    if(buf[0] + buf[1] + buf[2] + buf[3] == buf[4])
    {
        *humidity = (float)buf[0];
        *temperature = (float)buf[2];
        return 0;
    }
    
    return 1;
}
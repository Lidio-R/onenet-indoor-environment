#ifndef _USART_H_
#define _USART_H_


#include "stm32f10x.h"
#include "Common.h"

#define USART_DEBUG		USART2		//调试打印所使用的串口组


void Usart1_Init(unsigned int baud);

void Usart2_Init(unsigned int baud);

void Usart3_Init(unsigned int baud);

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);

void UsartPrintf_array(const uint8 *data, uint32 length) ;

void USART2_IRQHandler(void);

#endif

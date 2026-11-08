#ifndef _EZ_USART_H_
#define _EZ_USART_H_

#include "app_hal.h"

/*
 * Board usage:  USART2 PA2/PA3 -> HM-TRP radio (19200)
 *               USART3 PB10/PB11 -> FT232 debug port (115200)
 */
#define USART_3 3

void USART_INIT(USART_TypeDef *USART, uint32_t GPIORX, uint32_t GPIOTX, uint32_t RCC_AHB1Periph_GPIO,
                uint32_t RCC_APB1Periph_USART, uint32_t BAUDRATE, uint8_t USART_IRQn);
void USART_putc(USART_TypeDef *USARTx, volatile char c);
void USART_puts(USART_TypeDef *USARTx, volatile char *str);
char tostring(char str[], int num);
void eol(USART_TypeDef *USARTx);
void DISPLAY_INT2(USART_TypeDef *USARTx, uint32_t INT, uint16_t BASE, unsigned char EOL);

#endif

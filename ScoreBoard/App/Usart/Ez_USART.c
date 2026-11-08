/*
 * Ez_USART.c - serial helpers of the original firmware, now on top of HAL.
 * USART2 (radio, 19200) and USART3 (debug/FT232, 115200) are configured by
 * CubeMX; USART_INIT() only enables the RX-not-empty interrupt that the
 * original byte handlers in Comm.c rely on.
 */
#include "app_hal.h"
#include "app_main.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Ez_USART.h"


#define USART_TX_TIMEOUT_MS 100u


void USART_INIT(USART_TypeDef *USART, uint32_t GPIORX, uint32_t GPIOTX, uint32_t RCC_AHB1Periph_GPIO,
                uint32_t RCC_APB1Periph_USART, uint32_t BAUDRATE, uint8_t USART_IRQn)
{
  (void)GPIORX; (void)GPIOTX; (void)RCC_AHB1Periph_GPIO; (void)RCC_APB1Periph_USART;
  (void)BAUDRATE; (void)USART_IRQn;
  /* Pins, baud rate and NVIC come from CubeMX (MX_USARTx_UART_Init). */
  __HAL_UART_ENABLE_IT(APP_UartHandle(USART), UART_IT_RXNE);
}

void USART_puts(USART_TypeDef *USARTx, volatile char *str)
{
  const char *s = (const char *)str;
  HAL_UART_Transmit(APP_UartHandle(USARTx), (const uint8_t *)s, (uint16_t)strlen(s), USART_TX_TIMEOUT_MS);
}

void USART_putc(USART_TypeDef *USARTx, volatile char c)
{
  uint8_t ch = (uint8_t)c;
  HAL_UART_Transmit(APP_UartHandle(USARTx), &ch, 1, USART_TX_TIMEOUT_MS);
}


char tostring(char str[], int num)
{
  int i, rem, len = 0, n;
  n = num;
  while (n != 0)
  {
    len++;
    n /= 10;
  }
  for (i = 0; i < len; i++)
  {
    rem = num % 10;
    num = num / 10;
    str[len - (i + 1)] = rem + '0';
  }
  str[len] = '\0';
  return str[0];
}

void eol(USART_TypeDef *USARTx)
{
  USART_puts(USARTx, "\n\r");
}

void DISPLAY_INT2(USART_TypeDef *USARTx, uint32_t INT, uint16_t BASE, unsigned char EOL)
{
  char buffer[34];   /* 32 binary digits + sign + NUL */
  itoa(INT, buffer, BASE);
  if (EOL == 1)
  {
    eol(USARTx);
    USART_puts(USARTx, buffer);
    eol(USARTx);
  }
  else
  {
    USART_puts(USARTx, buffer);
  }
}

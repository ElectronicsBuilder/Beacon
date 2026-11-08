/*
 * uart_rx.h - receive ring buffers for USART2 (radio) and USART3 (debug port).
 *
 * The USART interrupts only store the received byte (APP_USARTx_IRQ, called from
 * the USER CODE section of the CubeMX IRQ handlers). UartRx_Poll() runs in the
 * App task: USART2 bytes go to the radio handler in Comm.c, USART3 bytes to
 * the console (Console/cli.c).
 */
#ifndef APP_UART_RX_H
#define APP_UART_RX_H

void APP_USART2_IRQ(void);   /* USART2 IRQ: store received byte */
void APP_USART3_IRQ(void);   /* USART3 IRQ: store received byte */
void UartRx_Poll(void);      /* App task: process all received bytes */

#endif

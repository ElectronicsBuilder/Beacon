/*
 * uart_rx.c - receive ring buffers for USART2 (radio) and USART3 (debug port).
 *
 * Single producer (the USART interrupt) / single consumer (the App task), so no
 * locking is needed: only the interrupt writes 'head', only the task writes 'tail'.
 */
#include "uart_rx.h"
#include "app_hal.h"
#include "Comm.h"
#include "cli.h"

#define UART_RX_SIZE 128u   /* bytes per port; commands are short */

typedef struct {
  volatile uint16_t head;   /* next write position (interrupt) */
  volatile uint16_t tail;   /* next read position (task) */
  uint8_t data[UART_RX_SIZE];
} RxRing;

static RxRing rx_usart2;
static RxRing rx_usart3;

static void P_Push(RxRing *r, uint8_t b)
{
  uint16_t next = (uint16_t)((r->head + 1u) % UART_RX_SIZE);
  if (next != r->tail)        /* drop the byte if the buffer is full */
  {
    r->data[r->head] = b;
    r->head = next;
  }
}

static int P_Pop(RxRing *r, uint8_t *b)
{
  if (r->tail == r->head)
  {
    return 0;
  }
  *b = r->data[r->tail];
  r->tail = (uint16_t)((r->tail + 1u) % UART_RX_SIZE);
  return 1;
}

static void P_Receive(USART_TypeDef *usart, RxRing *r)
{
  uint32_t sr = usart->SR;
  if (sr & (USART_SR_RXNE | USART_SR_ORE))
  {
    uint8_t b = (uint8_t)usart->DR;   /* reading SR then DR also clears an overrun */
    if (sr & USART_SR_RXNE)
    {
      P_Push(r, b);
    }
  }
}

void APP_USART2_IRQ(void)
{
  P_Receive(USART2, &rx_usart2);
}

void APP_USART3_IRQ(void)
{
  P_Receive(USART3, &rx_usart3);
}

void UartRx_Poll(void)
{
  uint8_t b;
  while (P_Pop(&rx_usart2, &b))
  {
    APP_USART2_Byte((char)b);
  }
  while (P_Pop(&rx_usart3, &b))
  {
    Cli_Input((char)b);       /* USART3 = console (legacy keys via the "keys" command) */
  }
}

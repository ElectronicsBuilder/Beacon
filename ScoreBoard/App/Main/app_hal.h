/*
 * app_hal.h
 *
 * Bridge between the 2016 scoreboard code (written for the StdPeriph library)
 * and the CubeMX/HAL project. Every ported module includes this header instead
 * of the old stm32f4xx_*.h SPL headers.
 */
#ifndef APP_HAL_H
#define APP_HAL_H

#include "main.h"      /* CubeMX: HAL + pin labels (LED_GREEN_Pin, OLED_CS_Pin, ...) */
#include "usart.h"     /* huart2 (radio), huart3 (debug) */
#include "spi.h"       /* hspi1  (OLED) */
#include "i2c.h"       /* hi2c1  (EEPROM) */
#include "tim.h"       /* htim2..htim5, htim7, htim8 */
#include "rtc.h"       /* hrtc */
#include "rng.h"       /* hrng */
#include <string.h>
#include <stdlib.h>

/* DMA handles created by CubeMX for the LED matrix (see tim.c) */
extern DMA_HandleTypeDef hdma_tim8_up;
extern DMA_HandleTypeDef hdma_tim8_ch1;

/* ---- SPL types still used by the old code ---- */
typedef enum { Bit_RESET = 0, Bit_SET } BitAction;

/* SPL clock-enable constants were passed to INIT_TIMER_CONFIG(); CubeMX now
 * enables the clocks, so the values are ignored. */
#define RCC_APB1Periph_TIM2   0u
#define RCC_APB1Periph_TIM3   0u
#define RCC_APB1Periph_TIM4   0u
#define RCC_APB1Periph_TIM5   0u
#define RCC_APB1Periph_TIM6   0u
#define RCC_APB1Periph_TIM7   0u

/* Map an old USARTx register pointer to its HAL handle. */
static inline UART_HandleTypeDef *APP_UartHandle(USART_TypeDef *usart)
{
  return (usart == USART2) ? &huart2 : &huart3;
}

/* Interrupt hooks, called from the USER CODE sections of Core/Src/stm32f4xx_it.c */
void APP_USART2_IRQ(void);          /* Usart/uart_rx.c: store received byte */
void APP_USART3_IRQ(void);          /* Usart/uart_rx.c: store received byte */
void APP_UART7_IRQ(void);           /* Ble/hm11.c: HM-11 receive / send */
void APP_DMA2_Stream1_IRQ(void);    /* Matrix/matrix.c: frame-end resync */

/* Run from the FreeRTOS default task (Core/Src/freertos.c). */
void App_Init(void);                /* start-up sequence (old main()) */
void App_Run(void);                 /* App task main loop, never returns */

#endif /* APP_HAL_H */

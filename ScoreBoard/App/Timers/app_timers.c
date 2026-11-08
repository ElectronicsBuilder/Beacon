/*
 * app_timers.c - start/stop the periodic interrupt timers (was TIMERS.c).
 *
 * Reproduces the StdPeriph behaviour the game logic depends on:
 *  - Starting a timer (SPL TIM_TimeBaseInit) generated an update event, so the
 *    handler ran once immediately.
 *  - Stopping a timer right after starting it (TIM7 does this to TIM5 every
 *    4 ms) still delivered that one pending tick, because the NVIC had already
 *    latched the interrupt. The LED matrix refresh (calculateLEDdata in the
 *    TIM5 handler) relies on this.
 */
#include "app_timers.h"

static TIM_HandleTypeDef *P_TimerHandle(TIM_TypeDef *tim)
{
  if (tim == TIM2) return &htim2;
  if (tim == TIM3) return &htim3;
  if (tim == TIM4) return &htim4;
  if (tim == TIM5) return &htim5;
  if (tim == TIM7) return &htim7;
  return NULL;
}

void INIT_TIMER_CONFIG(uint32_t TIMER_PERIPH, IRQn_Type TIMER_IRQ, TIM_TypeDef *TIM_TYPE, uint32_t TIME)
{
  (void)TIMER_PERIPH;
  (void)TIMER_IRQ;
  TIM_HandleTypeDef *htim = P_TimerHandle(TIM_TYPE);
  if (htim == NULL || TIME == 0u)
  {
    return;
  }
  __HAL_TIM_DISABLE(htim);
  __HAL_TIM_SET_AUTORELOAD(htim, TIME - 1u);
  __HAL_TIM_ENABLE_IT(htim, TIM_IT_UPDATE);
  htim->Instance->EGR = TIM_EGR_UG;      /* reload + update event -> one immediate tick (as SPL) */
  __HAL_TIM_ENABLE(htim);
}

void STOP_TIMER(TIM_TypeDef *TIM_TYPE)
{
  TIM_HandleTypeDef *htim = P_TimerHandle(TIM_TYPE);
  if (htim == NULL)
  {
    return;
  }
  __HAL_TIM_DISABLE(htim);
  if (__HAL_TIM_GET_FLAG(htim, TIM_FLAG_UPDATE) == RESET)
  {
    __HAL_TIM_DISABLE_IT(htim, TIM_IT_UPDATE);
  }
  /* else: leave the update interrupt enabled so the tick that is already
   * pending is still delivered once (the old firmware got it from the NVIC
   * latch). The counter is stopped, so no further ticks follow. */
}

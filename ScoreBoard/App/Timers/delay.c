/*
 * delay.c - blocking delays of the original firmware.
 * Milliseconds use HAL_Delay (TIM6 time base). Microseconds use the DWT cycle
 * counter instead of the old calibrated busy loop.
 */
#include "app_hal.h"
#include "delay.h"

void DELAY_INIT(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DELY_MICRO(uint32_t micros)
{
  const uint32_t start = DWT->CYCCNT;
  const uint32_t ticks = micros * (SystemCoreClock / 1000000u);
  while ((DWT->CYCCNT - start) < ticks)
  {
  }
}

void DELAY_MILLI(uint32_t millis)
{
  if (__get_IPSR() != 0u)
  {
    /* Called from an interrupt (the old timer ISRs draw on the displays):
     * the HAL tick (TIM6) cannot advance here, so busy-wait instead. */
    while (millis--)
    {
      DELY_MICRO(1000u);
    }
  }
  else
  {
    HAL_Delay(millis);
  }
}

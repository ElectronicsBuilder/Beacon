/*
 * delay.c - blocking delays of the original firmware.
 * Milliseconds use osDelay in the App task (HAL_Delay before the scheduler runs).
 * Microseconds use the DWT cycle counter instead of the old calibrated loop.
 */
#include "app_hal.h"
#include "delay.h"
#include "cmsis_os.h"

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
  if (osKernelGetState() == osKernelRunning && __get_IPSR() == 0u)
  {
    osDelay(millis);          /* App task: let the other tasks (USB) run */
  }
  else
  {
    HAL_Delay(millis);        /* before the scheduler starts */
  }
}

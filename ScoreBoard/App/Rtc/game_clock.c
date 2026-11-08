//--------------------------------------------------------------
// File     : game_clock.c
// Game timer (elapsed time since the game started), on the RTOS tick.
//
// The 2016 firmware used the RTC for this and reset it to 00:00:00 at every
// game start. The RTC now keeps the real date/time (Services/clock.c), so the
// game timer is software. The interface and the BCD outputs are unchanged.
//--------------------------------------------------------------
#include "game_clock.h"
#include "app_main.h"
#include "cmsis_os.h"

#define SECONDS_PER_DAY 86400u

static uint32_t start_ms;   /* tick at which the timer read 00:00:00 */

static uint32_t P_ElapsedSeconds(void)
{
  return ((osKernelGetTickCount() - start_ms) / 1000u) % SECONDS_PER_DAY;
}

static void P_SetElapsedSeconds(uint32_t seconds)
{
  start_ms = osKernelGetTickCount() - seconds * 1000u;
}

// Game start: timer to 00:00:00 and start the 1 s display refresh (TIM2 job).
void RTC_RESET_TIME(void)
{
  P_SetElapsedSeconds(0);
  INIT_TIMER_CONFIG(RTC_TIMER_PERIPH, RTC_TIMER_IRQ, RTC_TIM_TYPE, RTC_UPDATE_TIME);
}

void RTC_Configuration(void)
{
  RTC_RESET_TIME();
}

// Current timer value into SEC_H / MIN_H / HOUR_H (BCD), as before.
void RTC_TimeShow(void)
{
  uint32_t s = P_ElapsedSeconds();
  HOUR_H = ByteToBcd((uint8_t)(s / 3600u));
  MIN_H  = ByteToBcd((uint8_t)((s / 60u) % 60u));
  SEC_H  = ByteToBcd((uint8_t)(s % 60u));
}

// Set only the minutes (binary value), keep hours and seconds.
void RTC_SetMinutes(uint8_t minutes_bin)
{
  uint32_t s = P_ElapsedSeconds();
  P_SetElapsedSeconds((s / 3600u) * 3600u + (minutes_bin % 60u) * 60u + s % 60u);
}

// Set only the hours (BCD value), keep minutes and seconds.
void RTC_SetHours(uint8_t hours_bcd)
{
  uint32_t s = P_ElapsedSeconds();
  P_SetElapsedSeconds((Bcd2ToByte(hours_bcd) % 24u) * 3600u + s % 3600u);
}

uint8_t ByteToBcd(uint8_t Value)
{
  uint8_t bcdhigh = 0;
  while (Value >= 10)
  {
    bcdhigh++;
    Value -= 10;
  }
  return (uint8_t)((bcdhigh << 4) | Value);
}

uint8_t Bcd2ToByte(uint8_t Value)
{
  uint8_t tmp = (uint8_t)(((Value & 0xF0u) >> 4) * 10u);
  return (uint8_t)(tmp + (Value & 0x0Fu));
}

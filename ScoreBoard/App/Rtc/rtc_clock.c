//--------------------------------------------------------------
// File     : rtc_clock.c
// Game clock based on the internal RTC (HAL version).
//
// The RTC runs from the LSI (PC14/PC15 are used by the LED matrix, so there
// is no 32 kHz crystal). Prescalers 127/255 give a 1 Hz calendar tick.
// The time is shown in BCD in SEC_H / MIN_H / HOUR_H, as before.
//--------------------------------------------------------------
#include "rtc_clock.h"
#include "app_main.h"
#include "Ez_USART.h"

static void P_RTC_SetTimeDate(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t format)
{
  RTC_TimeTypeDef t = {0};
  RTC_DateTypeDef d = {0};

  t.Hours = hours;
  t.Minutes = minutes;
  t.Seconds = seconds;
  t.TimeFormat = RTC_HOURFORMAT12_AM;
  t.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  t.StoreOperation = RTC_STOREOPERATION_RESET;
  HAL_RTC_SetTime(&hrtc, &t, format);

  /* Same fixed date as the original firmware: Tuesday 14 July 2015 */
  d.WeekDay = RTC_WEEKDAY_TUESDAY;
  d.Month = RTC_MONTH_JULY;
  d.Date = 0x14;
  d.Year = 0x15;
  HAL_RTC_SetDate(&hrtc, &d, RTC_FORMAT_BCD);
}

// Reset the clock to 00:00:00 and start the 1 s refresh timer (TIM2).
void RTC_RESET_TIME(void)
{
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, FIRST_DATA);
  P_RTC_SetTimeDate(0x00, 0x00, 0x00, RTC_FORMAT_BCD);
  INIT_TIMER_CONFIG(RTC_TIMER_PERIPH, RTC_TIMER_IRQ, RTC_TIM_TYPE, RTC_UPDATE_TIME);
}

// Start-up configuration. The original also always started from 00:00:00.
void RTC_Configuration(void)
{
  RTC_RESET_TIME();
}

// Read the current time into SEC_H / MIN_H / HOUR_H (BCD).
void RTC_TimeShow(void)
{
  RTC_TimeTypeDef t;
  RTC_DateTypeDef d;

  HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BCD);
  HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BCD);   /* must follow GetTime to unlock the shadow registers */

  SEC_H = t.Seconds;
  MIN_H = t.Minutes;
  HOUR_H = t.Hours;

}

// Set only the minutes (binary value), keep hours and seconds.
void RTC_SetMinutes(uint8_t minutes_bin)
{
  RTC_TimeTypeDef t;
  RTC_DateTypeDef d;
  HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
  P_RTC_SetTimeDate(t.Hours, minutes_bin, t.Seconds, RTC_FORMAT_BIN);
}

// Set only the hours (BCD value), keep minutes and seconds.
void RTC_SetHours(uint8_t hours_bcd)
{
  RTC_TimeTypeDef t;
  RTC_DateTypeDef d;
  HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BCD);
  HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BCD);
  P_RTC_SetTimeDate(hours_bcd, t.Minutes, t.Seconds, RTC_FORMAT_BCD);
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

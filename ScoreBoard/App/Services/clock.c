/*
 * clock.c - wall clock (date and time) on the STM32 RTC (HAL, binary format).
 */
#include "clock.h"
#include "app_hal.h"
#include "clock_calc.h"

#define CLOCK_SET_MARKER  0xC10Cu        /* backup register DR1: time was set */

int Clock_IsSet(void)
{
  return HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) == CLOCK_SET_MARKER;
}

void Clock_Get(ClockTime *t)
{
  RTC_TimeTypeDef tm;
  RTC_DateTypeDef dt;
  HAL_RTC_GetTime(&hrtc, &tm, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &dt, RTC_FORMAT_BIN);   /* must follow GetTime (shadow registers) */
  t->year = (uint16_t)(2000u + dt.Year);
  t->month = dt.Month;
  t->day = dt.Date;
  t->hour = tm.Hours;
  t->minute = tm.Minutes;
  t->second = tm.Seconds;
  t->weekday = dt.WeekDay;
}

int Clock_Set(const ClockTime *t)
{
  if (!ClockCalc_IsValid(t))
  {
    return -1;
  }
  RTC_TimeTypeDef tm = {0};
  RTC_DateTypeDef dt = {0};
  tm.Hours = t->hour;
  tm.Minutes = t->minute;
  tm.Seconds = t->second;
  tm.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  tm.StoreOperation = RTC_STOREOPERATION_RESET;
  dt.Year = (uint8_t)(t->year - 2000u);
  dt.Month = t->month;
  dt.Date = t->day;
  dt.WeekDay = ClockCalc_Weekday(t->year, t->month, t->day);
  if (HAL_RTC_SetTime(&hrtc, &tm, RTC_FORMAT_BIN) != HAL_OK ||
      HAL_RTC_SetDate(&hrtc, &dt, RTC_FORMAT_BIN) != HAL_OK)
  {
    return -1;
  }
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, CLOCK_SET_MARKER);
  return 0;
}

int Clock_Parse(const char *date, const char *time, ClockTime *t)
{
  Clock_Get(t);                       /* fields that are not given stay as they are */
  return ClockCalc_Parse(date, time, t);
}

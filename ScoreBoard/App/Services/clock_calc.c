/*
 * clock_calc.c - date/time arithmetic and parsing (no hardware access, so it
 * is also built and tested on the PC: tests/host).
 */
#include "clock_calc.h"
#include <stdio.h>

int ClockCalc_DaysInMonth(uint16_t year, uint8_t month)
{
  static const uint8_t days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (month == 2u && (year % 4u) == 0u)   /* 2000..2099: every 4th year is a leap year */
  {
    return 29;
  }
  return days[month - 1u];
}

/* ISO weekday, 1 = Monday .. 7 = Sunday (Sakamoto's algorithm). */
uint8_t ClockCalc_Weekday(uint16_t y, uint8_t m, uint8_t d)
{
  static const uint8_t t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  if (m < 3u) y--;
  uint32_t w = (y + y / 4u - y / 100u + y / 400u + t[m - 1u] + d) % 7u;   /* 0 = Sunday */
  return (uint8_t)(w == 0u ? 7u : w);
}

int ClockCalc_IsValid(const ClockTime *t)
{
  return t->year >= 2000u && t->year <= 2099u &&
         t->month >= 1u && t->month <= 12u &&
         t->day >= 1u && t->day <= ClockCalc_DaysInMonth(t->year, t->month) &&
         t->hour <= 23u && t->minute <= 59u && t->second <= 59u;
}

int ClockCalc_Parse(const char *date, const char *time, ClockTime *t)
{
  if (date != NULL)
  {
    unsigned y, mo, d;
    char extra;
    if (sscanf(date, "%4u-%2u-%2u%c", &y, &mo, &d, &extra) != 3) return -1;
    t->year = (uint16_t)y; t->month = (uint8_t)mo; t->day = (uint8_t)d;
  }
  if (time != NULL)
  {
    unsigned h, mi, s = 0;
    char extra;
    int n = sscanf(time, "%2u:%2u:%2u%c", &h, &mi, &s, &extra);
    if (n != 2 && n != 3) return -1;   /* seconds optional */
    t->hour = (uint8_t)h; t->minute = (uint8_t)mi; t->second = (uint8_t)s;
  }
  if (!ClockCalc_IsValid(t)) return -1;
  t->weekday = ClockCalc_Weekday(t->year, t->month, t->day);
  return 0;
}

void Clock_Format(const ClockTime *t, char *buf, size_t len)
{
  snprintf(buf, len, "%04u-%02u-%02u %02u:%02u:%02u",
           t->year, t->month, t->day, t->hour, t->minute, t->second);
}

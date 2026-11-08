/*
 * clock_calc.h - date/time arithmetic and parsing without hardware access.
 */
#ifndef APP_CLOCK_CALC_H
#define APP_CLOCK_CALC_H

#include "clock.h"

int     ClockCalc_DaysInMonth(uint16_t year, uint8_t month);   /* 2000..2099 */
uint8_t ClockCalc_Weekday(uint16_t year, uint8_t month, uint8_t day);   /* 1 = Mon .. 7 = Sun */
int     ClockCalc_IsValid(const ClockTime *t);

/* Parse "YYYY-MM-DD" and/or "HH:MM[:SS]" into *t (fields not given are kept).
 * Returns 0, or -1 if the text or the resulting date/time is invalid. */
int     ClockCalc_Parse(const char *date, const char *time, ClockTime *t);

#endif

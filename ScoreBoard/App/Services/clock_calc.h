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

/* Seconds since 2000-01-01 00:00:00 and back (2000..2099) */
uint32_t ClockCalc_ToSeconds(const ClockTime *t);
void     ClockCalc_FromSeconds(uint32_t secs, ClockTime *t);

/* "HH:MM:SS" of the time within the day into hms; returns the whole days */
uint32_t ClockCalc_FormatDuration(uint32_t secs, char *hms, size_t len);

/* "1h30m", "2d", "45s", "90" (bare number = minutes) -> seconds; 0 = ok */
int      ClockCalc_ParseDuration(const char *s, uint32_t *secs);

#endif

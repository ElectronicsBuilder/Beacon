/*
 * clock.h - wall clock (date and time) on the STM32 RTC.
 *
 * The RTC runs from the LSI oscillator (PC14/PC15 are used by the LED matrix,
 * so there is no 32 kHz crystal). It keeps counting across MCU resets while
 * the board is powered, but not across a power loss; the GUI sets the time on
 * every connect. Clock_IsSet() reports whether the time was set since power-up.
 */
#ifndef APP_CLOCK_H
#define APP_CLOCK_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint16_t year;     /* 2000..2099 */
  uint8_t  month;    /* 1..12 */
  uint8_t  day;      /* 1..31 */
  uint8_t  hour;     /* 0..23 */
  uint8_t  minute;   /* 0..59 */
  uint8_t  second;   /* 0..59 */
  uint8_t  weekday;  /* 1 = Monday .. 7 = Sunday */
} ClockTime;

int  Clock_IsSet(void);
void Clock_Get(ClockTime *t);
int  Clock_Set(const ClockTime *t);                 /* 0 = ok, -1 = invalid */

/* Parse "YYYY-MM-DD" and/or "HH:MM:SS" (either may be NULL: keeps the current
 * value). Returns 0 and fills *t, or -1 if a field is invalid. */
int  Clock_Parse(const char *date, const char *time, ClockTime *t);

/* "YYYY-MM-DD HH:MM:SS" */
void Clock_Format(const ClockTime *t, char *buf, size_t len);

#endif

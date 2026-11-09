/*
 * countdown.h - countdown to a date/time and count-up from a date/time.
 *
 * Shown by the "countdown" and "countup" modes (mode_countdown.c): big
 * HH:MM:SS, and a text line with the days ("12D") and the label. When the
 * countdown reaches zero it shows 00:00:00 in red and the end message.
 * Settings are kept in the EEPROM (saved 1 s after the last change).
 * Times are seconds since 2000-01-01 (ClockCalc_ToSeconds), 0 = not set.
 */
#ifndef APP_COUNTDOWN_H
#define APP_COUNTDOWN_H

#include <stdint.h>
#include "end_plan.h"

#define TIMER_TEXT_MAX 48

typedef struct {
  uint32_t cd_target;                     /* countdown end */
  uint32_t cu_start;                      /* count-up start */
  uint32_t cu_stopped_at;                 /* 0 = running, else frozen at this time */
  char cd_label[TIMER_TEXT_MAX + 1];      /* shown while counting down */
  char cd_end[TIMER_TEXT_MAX + 1];        /* shown at zero ("" = none) */
  char cu_label[TIMER_TEXT_MAX + 1];      /* shown while counting up */
} TimerConfig;

void Timer_Load(void);                    /* at boot */
void Timer_Poll(void);                    /* App task: delayed EEPROM save */
const TimerConfig *Timer_Config(void);

void Timer_SetCountdown(uint32_t target);
void Timer_SetCountdownLabel(const char *text);
void Timer_SetCountdownEnd(const char *text);
void Timer_StartCountup(uint32_t start);
void Timer_StopCountup(void);
void Timer_ResumeCountup(void);
void Timer_SetCountupLabel(const char *text);

/* Current values: remaining / elapsed seconds; return 0, or -1 if not set or
 * the clock is not set. */
int Timer_Remaining(uint32_t *secs);      /* 0 once the target has passed */
int Timer_Elapsed(uint32_t *secs);

/* After zero: scene, then hold or the end screen (see end_plan.h); saved */
const EndPlan *Timer_EndPlan(void);
void Timer_SetEndPlan(const EndPlan *p);

#endif

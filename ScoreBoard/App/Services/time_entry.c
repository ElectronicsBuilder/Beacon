/*
 * time_entry.c - date/time editor for keyboard entry. See time_entry.h.
 * Pure logic, unit tested on the PC (tests/host).
 */
#include "time_entry.h"
#include "clock_calc.h"
#include "keys.h"
#include <stdio.h>
#include <string.h>

static int P_IsDigitPos(uint8_t pos)
{
  return pos < TIME_ENTRY_LEN && pos != 4 && pos != 7 && pos != 10 && pos != 13;
}

static void P_Right(TimeEntry *e)
{
  uint8_t p = e->pos;
  do { p++; } while (p < TIME_ENTRY_LEN && !P_IsDigitPos(p));
  if (p < TIME_ENTRY_LEN) e->pos = p;
}

static void P_Left(TimeEntry *e)
{
  int p = e->pos;
  do { p--; } while (p >= 0 && !P_IsDigitPos((uint8_t)p));
  if (p >= 0) e->pos = (uint8_t)p;
}

void TimeEntry_Start(TimeEntry *e, const ClockTime *t)
{
  snprintf(e->text, sizeof e->text, "%04u-%02u-%02u %02u:%02u",
           (unsigned)(t->year % 10000u), (unsigned)(t->month % 100u), (unsigned)(t->day % 100u),
           (unsigned)(t->hour % 100u), (unsigned)(t->minute % 100u));
  e->pos = 0;
}

TimeEntryState TimeEntry_Key(TimeEntry *e, uint8_t key)
{
  if (key >= '0' && key <= '9')
  {
    e->text[e->pos] = (char)key;
    P_Right(e);
  }
  else if (key == KB_RIGHT || key == ' ')
  {
    P_Right(e);
  }
  else if (key == KB_LEFT || key == KB_BACK || key == KB_DEL)
  {
    P_Left(e);
  }
  else if (key == KB_ENTER || key == '\n')
  {
    return TE_DONE;
  }
  else if (key == KB_ESC)
  {
    return TE_CANCEL;
  }
  return TE_EDITING;
}

int TimeEntry_Get(const TimeEntry *e, ClockTime *t)
{
  char date[11], time[6];
  memcpy(date, e->text, 10);
  date[10] = '\0';
  memcpy(time, e->text + 11, 5);
  time[5] = '\0';
  return ClockCalc_Parse(date, time, t);
}

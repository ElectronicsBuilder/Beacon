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

/* next digit to the right; at the end: stay (typing) or wrap to the start (arrow) */
static void P_Right(TimeEntry *e, int wrap)
{
  uint8_t p = e->pos;
  do { p++; } while (p < TIME_ENTRY_LEN && !P_IsDigitPos(p));
  if (p < TIME_ENTRY_LEN) e->pos = p;
  else if (wrap) e->pos = 0;
}

static void P_Left(TimeEntry *e)
{
  int p = e->pos;
  do { p--; } while (p >= 0 && !P_IsDigitPos((uint8_t)p));
  e->pos = (p >= 0) ? (uint8_t)p : (uint8_t)(TIME_ENTRY_LEN - 1);
}

static unsigned P_Field(const TimeEntry *e, int start, int len)
{
  unsigned v = 0;
  for (int i = 0; i < len; i++)
  {
    char c = e->text[start + i];
    v = v * 10u + (unsigned)((c >= '0' && c <= '9') ? c - '0' : 0);
  }
  return v;
}

/* Up/Down: step the field under the cursor, wrapping within its range */
static void P_Step(TimeEntry *e, int delta)
{
  static const struct { uint8_t start, len; } fields[] = { {0, 4}, {5, 2}, {8, 2}, {11, 2}, {14, 2} };
  int f = (e->pos < 4) ? 0 : (e->pos < 7) ? 1 : (e->pos < 10) ? 2 : (e->pos < 13) ? 3 : 4;
  int lo, hi;
  switch (f)
  {
    case 0:  lo = 2000; hi = 2099; break;
    case 1:  lo = 1;    hi = 12;   break;
    case 2:
    {
      unsigned y = P_Field(e, 0, 4), m = P_Field(e, 5, 2);
      lo = 1;
      hi = (y >= 2000u && y <= 2099u && m >= 1u && m <= 12u) ? ClockCalc_DaysInMonth((uint16_t)y, (uint8_t)m) : 31;
      break;
    }
    case 3:  lo = 0;    hi = 23;   break;
    default: lo = 0;    hi = 59;   break;
  }
  int v = (int)P_Field(e, fields[f].start, fields[f].len) + delta;
  if (v > hi) v = lo;
  if (v < lo) v = hi;
  char buf[6];
  snprintf(buf, sizeof buf, (fields[f].len == 4) ? "%04d" : "%02d", v);
  memcpy(&e->text[fields[f].start], buf, fields[f].len);
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
    P_Right(e, 0);
  }
  else if (key == KB_RIGHT || key == ' ')
  {
    P_Right(e, 1);
  }
  else if (key == KB_UP || key == '+' || key == '=')
  {
    P_Step(e, +1);
  }
  else if (key == KB_DOWN || key == '-' || key == '_')
  {
    P_Step(e, -1);
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

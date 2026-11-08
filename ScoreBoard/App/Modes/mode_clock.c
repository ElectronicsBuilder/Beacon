/*
 * mode_clock.c - wall clock: big HH:MM:SS with the weekday and date below.
 */
#include "mode.h"
#include "clock.h"
#include "layout.h"
#include "FONT.h"
#include <stdio.h>
#include <string.h>

#define CLOCK_POLL_MS 100u

static uint32_t poll_count;
static int8_t last_second;

static void P_Draw(void)
{
  char big[12];
  char sub[20];

  if (Clock_IsSet())
  {
    static const char *const days[] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
    ClockTime t;
    Clock_Get(&t);
    if ((int8_t)t.second == last_second)
    {
      return;
    }
    last_second = (int8_t)t.second;
    snprintf(big, sizeof big, "%02u:%02u:%02u", t.hour, t.minute, t.second);
    snprintf(sub, sizeof sub, "%s %04u-%02u-%02u",
             (t.weekday >= 1 && t.weekday <= 7) ? days[t.weekday - 1] : "---",
             t.year, t.month, t.day);
  }
  else
  {
    if (last_second == -2)
    {
      return;
    }
    last_second = -2;
    strcpy(big, "--:--:--");
    strcpy(sub, "SET THE TIME");
  }

  CHANGE_FONT(WHITE);          /* white reads the same on both panel halves */
  calculateLEDStringScaled(big, LAYOUT_CENTER_X(strlen(big), LAYOUT_BIG_CHAR_W), LAYOUT_BIG_Y,
                           LAYOUT_BIG_FONT, LAYOUT_BIG_SCALE);
#if HEIGHT == 64
  CHANGE_FONT(BLUE);
#else
  CHANGE_FONT(WHITE);          /* upper band: only white is readable with the B1/G1 fault */
#endif
  calculateLEDString("                ", 0, LAYOUT_BIG_SUB_Y, Font8x8);
  calculateLEDString(sub, LAYOUT_CENTER_X(strlen(sub), 8), LAYOUT_BIG_SUB_Y, Font8x8);
}

static void P_Enter(void)
{
  poll_count = 0;
  last_second = -1;
  P_Draw();
}

static void P_Exit(void)
{
}

static void P_Tick(void)
{
  if (++poll_count >= CLOCK_POLL_MS)
  {
    poll_count = 0;
    P_Draw();
  }
}

static void P_Key(uint8_t key)
{
  (void)key;                   /* time entry from the keyboard: M3 */
}

const AppMode mode_clock = {
  "clock", "Clock", MODE_KIND_DISPLAY, P_Enter, P_Exit, P_Tick, P_Key
};

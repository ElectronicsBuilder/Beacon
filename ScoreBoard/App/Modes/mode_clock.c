/*
 * mode_clock.c - wall clock: big HH:MM:SS, the weekday and date, messages.
 * What goes on each display line is set with "screen clock ..." (ui_bands).
 *
 * Keyboard: T or Enter opens the date/time editor (seconds are set to 0).
 */
#include "mode.h"
#include "mode_ui.h"
#include "ui_bands.h"
#include "clock.h"
#include "keys.h"
#include "log.h"
#include <stdio.h>

#define CLOCK_POLL_MS 100u

static uint32_t poll_count;
static char big[12];
static char sub[20];

static void P_Update(void)
{
  if (Clock_IsSet())
  {
    static const char *const days[] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
    ClockTime t;
    Clock_Get(&t);
    snprintf(big, sizeof big, "%02u:%02u:%02u", t.hour, t.minute, t.second);
    snprintf(sub, sizeof sub, "%s %04u-%02u-%02u",
             (t.weekday >= 1 && t.weekday <= 7) ? days[t.weekday - 1] : "---",
             t.year, t.month, t.day);
  }
  else
  {
    snprintf(big, sizeof big, "--:--:--");
    snprintf(sub, sizeof sub, "PRESS T TO SET");
  }
}

static void P_Redraw(void)
{
  calculateClearImage();
  P_Update();
  Bands_Enter(SCREEN_CLOCK);
}

static void P_Enter(void)
{
  poll_count = 0;
  P_Redraw();
}

static void P_Exit(void)
{
  TimeEdit_Stop();
}

static void P_Tick(void)
{
  if (TimeEdit_Active())
  {
    TimeEdit_Tick();
    return;
  }
  if (++poll_count >= CLOCK_POLL_MS)
  {
    poll_count = 0;
    P_Update();
  }
  Bands_Tick(big, WHITE, sub, 1);
}

static void P_Key(uint8_t key)
{
  if (TimeEdit_Active())
  {
    ClockTime t;
    TimeEntryState st = TimeEdit_Key(key, &t);
    if (st == TE_DONE && Clock_Set(&t) == 0)
    {
      char text[24];
      Clock_Format(&t, text, sizeof text);
      LOG_I("Time set from the keyboard: %s", text);
    }
    if (st != TE_EDITING)
    {
      TimeEdit_Stop();
      P_Redraw();
    }
  }
  else if (key == 't' || key == 'T' || key == KB_ENTER)
  {
    ClockTime t;
    Clock_Get(&t);
    if (!Clock_IsSet() || t.year < 2026)
    {
      t = (ClockTime){ 2026, 1, 1, 0, 0, 0, 4 };
    }
    TimeEdit_Start("SET DATE & TIME", &t);
  }
}

const AppMode mode_clock = {
  "clock", "Clock", MODE_KIND_DISPLAY, P_Enter, P_Exit, P_Tick, P_Key
};

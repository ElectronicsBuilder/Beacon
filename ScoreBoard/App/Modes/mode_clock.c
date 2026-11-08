/*
 * mode_clock.c - wall clock: big HH:MM:SS with the weekday and date.
 *
 * Keyboard: T or Enter opens the date/time editor (seconds are set to 0).
 */
#include "mode.h"
#include "mode_ui.h"
#include "ui_ticker.h"
#include "clock.h"
#include "keys.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

#define CLOCK_POLL_MS 100u

static uint32_t poll_count;
static int8_t last_second;
static TextLine sub_line;
static Ticker ticker;
static char own_text[20];               /* date line (messages may take turns) */

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
    snprintf(big, sizeof big, "--:--:--");
    snprintf(sub, sizeof sub, "PRESS T TO SET");
  }
  ModeUi_DrawBig(big, WHITE);
  strcpy(own_text, sub);
  if (!ticker.showing) TextLine_Set(&sub_line, sub, MODE_UI_TEXT_COLOR);
}

static void P_Redraw(void)
{
  last_second = -1;
  TextLine_Init(&sub_line, LAYOUT_BIG_SUB_Y, MODE_UI_TEXT_COLOR);
  Ticker_Reset(&ticker);
  P_Draw();
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
  MatrixColor color = MODE_UI_TEXT_COLOR;
  const char *text = Ticker_Tick(&ticker, &sub_line, own_text, &color);
  TextLine_Set(&sub_line, text, color);
  TextLine_Tick(&sub_line);
  if (++poll_count >= CLOCK_POLL_MS)
  {
    poll_count = 0;
    P_Draw();
  }
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

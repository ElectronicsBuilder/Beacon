/*
 * mode_clock.c - wall clock: big HH:MM:SS with the weekday and date.
 *
 * Keyboard: T or Enter opens the date/time editor ("YYYY-MM-DD HH:MM"):
 * digits overwrite, Left/Right/Backspace move, Enter saves (seconds = 0),
 * ESC cancels.
 */
#include "mode.h"
#include "clock.h"
#include "time_entry.h"
#include "keys.h"
#include "layout.h"
#include "FONT.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

#define CLOCK_POLL_MS     100u
#define BLINK_MS          400u
#define INVALID_SHOW_MS   2000u

static uint32_t poll_count;
static int8_t last_second;
static uint8_t editing;
static TimeEntry entry;
static uint32_t edit_ms;           /* time in the editor (blink, messages) */
static uint32_t invalid_until_ms;

/* ---------------------------------------------------------------- clock view */

static void P_DrawClock(void)
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
    strcpy(sub, "PRESS T TO SET");
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

/* ---------------------------------------------------------------- editor view */

static void P_DrawEditor(void)
{
  int invalid = (int32_t)(invalid_until_ms - edit_ms) > 0;
  const char *title = invalid ? "INVALID DATE" : "SET DATE & TIME";

  CHANGE_FONT(invalid ? RED : WHITE);
  calculateLEDString("                ", 0, LAYOUT_EDIT_TITLE_Y, Font8x8);
  calculateLEDString((char *)title, LAYOUT_CENTER_X(strlen(title), 8), LAYOUT_EDIT_TITLE_Y, Font8x8);

  CHANGE_FONT(BLUE);
  calculateLEDString(entry.text, 0, LAYOUT_EDIT_Y, Font8x8);
  /* cursor: the digit in red, blinking with an underscore */
  char cursor[2] = { ((edit_ms / BLINK_MS) % 2u) ? '_' : entry.text[entry.pos], '\0' };
  CHANGE_FONT(RED);
  calculateLEDString(cursor, entry.pos * 8, LAYOUT_EDIT_Y, Font8x8);
  CHANGE_FONT(BLUE);
}

static void P_StartEditor(void)
{
  ClockTime t;
  Clock_Get(&t);
  if (!Clock_IsSet() || t.year < 2026)
  {
    t.year = 2026; t.month = 1; t.day = 1; t.hour = 0; t.minute = 0;
  }
  TimeEntry_Start(&entry, &t);
  editing = 1;
  edit_ms = 0;
  invalid_until_ms = 0;
  calculateClearImage();
  P_DrawEditor();
}

static void P_StopEditor(void)
{
  editing = 0;
  calculateClearImage();
  last_second = -1;
  P_DrawClock();
}

static void P_EditorKey(uint8_t key)
{
  /* restart the blink with the cursor digit visible; any key clears "INVALID" */
  edit_ms = (edit_ms / (2u * BLINK_MS) + 1u) * (2u * BLINK_MS);
  invalid_until_ms = 0;
  switch (TimeEntry_Key(&entry, key))
  {
    case TE_DONE:
    {
      ClockTime t;
      Clock_Get(&t);
      if (TimeEntry_Get(&entry, &t) == 0 && Clock_Set(&t) == 0)
      {
        char text[24];
        Clock_Format(&t, text, sizeof text);
        LOG_I("Time set from the keyboard: %s", text);
        P_StopEditor();
        return;
      }
      invalid_until_ms = edit_ms + INVALID_SHOW_MS;
      break;
    }
    case TE_CANCEL:
      P_StopEditor();
      return;
    default:
      break;
  }
  P_DrawEditor();
}

/* ---------------------------------------------------------------- mode */

static void P_Enter(void)
{
  poll_count = 0;
  editing = 0;
  last_second = -1;
  P_DrawClock();
}

static void P_Exit(void)
{
  editing = 0;
}

static void P_Tick(void)
{
  if (editing)
  {
    uint32_t before = edit_ms / BLINK_MS;
    int was_invalid = (int32_t)(invalid_until_ms - edit_ms) > 0;
    edit_ms++;
    int is_invalid = (int32_t)(invalid_until_ms - edit_ms) > 0;
    if (edit_ms / BLINK_MS != before || was_invalid != is_invalid)
    {
      P_DrawEditor();
    }
    return;
  }
  if (++poll_count >= CLOCK_POLL_MS)
  {
    poll_count = 0;
    P_DrawClock();
  }
}

static void P_Key(uint8_t key)
{
  if (editing)
  {
    P_EditorKey(key);
  }
  else if (key == 't' || key == 'T' || key == KB_ENTER)
  {
    P_StartEditor();
  }
}

const AppMode mode_clock = {
  "clock", "Clock", MODE_KIND_DISPLAY, P_Enter, P_Exit, P_Tick, P_Key
};

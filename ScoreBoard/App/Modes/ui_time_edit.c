/*
 * ui_time_edit.c - date/time editor on the matrix. See ui_time_edit.h.
 */
#include "ui_time_edit.h"
#include "matrix.h"
#include "layout.h"
#include "FONT.h"
#include <string.h>

#define BLINK_MS          400u
#define INVALID_SHOW_MS   2000u

static TimeEntry entry;
static const char *title_text;
static uint8_t active;
static uint32_t edit_ms;
static uint32_t invalid_until_ms;

static int P_Invalid(void)
{
  return (int32_t)(invalid_until_ms - edit_ms) > 0;
}

static void P_Draw(void)
{
  const char *title = P_Invalid() ? "INVALID DATE" : title_text;

  CHANGE_FONT(P_Invalid() ? RED : WHITE);
  calculateLEDString("                ", 0, LAYOUT_EDIT_TITLE_Y, Font8x8);
  calculateLEDString((char *)title, LAYOUT_CENTER_X(strlen(title), 8), LAYOUT_EDIT_TITLE_Y, Font8x8);

  CHANGE_FONT(BLUE);
  calculateLEDString(entry.text, 0, LAYOUT_EDIT_Y, Font8x8);
  char cursor[2] = { ((edit_ms / BLINK_MS) % 2u) ? '_' : entry.text[entry.pos], '\0' };
  CHANGE_FONT(RED);
  calculateLEDString(cursor, entry.pos * 8, LAYOUT_EDIT_Y, Font8x8);
  CHANGE_FONT(BLUE);
}

void TimeEdit_Start(const char *title, const ClockTime *initial)
{
  title_text = title;
  TimeEntry_Start(&entry, initial);
  active = 1;
  edit_ms = 0;
  invalid_until_ms = 0;
  calculateClearImage();
  P_Draw();
}

int TimeEdit_Active(void)
{
  return active;
}

void TimeEdit_Stop(void)
{
  if (active)
  {
    active = 0;
    calculateClearImage();
  }
}

void TimeEdit_Tick(void)
{
  if (!active) return;
  uint32_t phase = edit_ms / BLINK_MS;
  int was_invalid = P_Invalid();
  edit_ms++;
  if (edit_ms / BLINK_MS != phase || was_invalid != P_Invalid())
  {
    P_Draw();
  }
}

TimeEntryState TimeEdit_Key(uint8_t key, ClockTime *result)
{
  if (!active) return TE_CANCEL;
  /* restart the blink with the cursor digit visible; any key clears "INVALID" */
  edit_ms = (edit_ms / (2u * BLINK_MS) + 1u) * (2u * BLINK_MS);
  invalid_until_ms = 0;
  TimeEntryState st = TimeEntry_Key(&entry, key);
  if (st == TE_DONE)
  {
    if (TimeEntry_Get(&entry, result) == 0)
    {
      return TE_DONE;
    }
    invalid_until_ms = edit_ms + INVALID_SHOW_MS;
    st = TE_EDITING;
  }
  else if (st == TE_CANCEL)
  {
    TimeEdit_Stop();
    return TE_CANCEL;
  }
  P_Draw();
  return st;
}

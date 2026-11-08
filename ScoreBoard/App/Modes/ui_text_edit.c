/*
 * ui_text_edit.c - one-line text editor on the matrix. See ui_text_edit.h.
 */
#include "ui_text_edit.h"
#include "matrix.h"
#include "layout.h"
#include "FONT.h"
#include <string.h>

#define BLINK_MS   400u
#define VISIBLE    ((WIDTH / 8) - 1)     /* characters before the cursor */

static TextEntry entry;
static const char *title_text;
static uint8_t active;
static uint32_t edit_ms;

static void P_Draw(void)
{
  Matrix_SetTextColor(MTX_C_WHITE);
  calculateLEDString("                ", 0, LAYOUT_EDIT_TITLE_Y, Font8x8);
  calculateLEDString((char *)title_text, LAYOUT_CENTER_X(strlen(title_text), 8), LAYOUT_EDIT_TITLE_Y, Font8x8);

  char shown[VISIBLE + 2];
  const char *tail = entry.text + (entry.len > VISIBLE ? entry.len - VISIBLE : 0);
  size_t n = strlen(tail);
  memcpy(shown, tail, n);
  shown[n] = ((edit_ms / BLINK_MS) % 2u) ? ' ' : '_';
  shown[n + 1] = '\0';
  Matrix_SetTextColor(MTX_C_GREEN);
  calculateLEDString("                ", 0, LAYOUT_EDIT_Y, Font8x8);
  calculateLEDString(shown, 0, LAYOUT_EDIT_Y, Font8x8);
  CHANGE_FONT(BLUE);
}

void TextEdit_Start(const char *title, const char *initial, uint8_t max)
{
  title_text = title;
  TextEntry_Start(&entry, initial, max);
  active = 1;
  edit_ms = 0;
  calculateClearImage();
  P_Draw();
}

int TextEdit_Active(void)  { return active; }
const char *TextEdit_Text(void) { return entry.text; }

void TextEdit_Stop(void)
{
  if (active)
  {
    active = 0;
    calculateClearImage();
  }
}

void TextEdit_Tick(void)
{
  if (!active) return;
  uint32_t phase = edit_ms / BLINK_MS;
  edit_ms++;
  if (edit_ms / BLINK_MS != phase) P_Draw();
}

TimeEntryState TextEdit_Key(uint8_t key)
{
  if (!active) return TE_CANCEL;
  edit_ms = (edit_ms / (2u * BLINK_MS) + 1u) * (2u * BLINK_MS);   /* cursor visible now */
  TimeEntryState st = TextEntry_Key(&entry, key);
  if (st == TE_CANCEL)
  {
    TextEdit_Stop();
    return st;
  }
  P_Draw();
  return st;
}

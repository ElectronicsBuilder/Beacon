/*
 * ui_text_line.c - centred or scrolling text line. See ui_text_line.h.
 */
#include "ui_text_line.h"
#include "matrix.h"
#include "layout.h"
#include "FONT.h"
#include <string.h>

#define SCROLL_MS     30u
#define LINE_CHARS    (WIDTH / 8)

static void P_Clear(const TextLine *l)
{
  calculateLEDString("                ", 0, l->y, Font8x8);
}

void TextLine_Redraw(TextLine *l)
{
  size_t n = strlen(l->text);
  CHANGE_FONT(l->color);
  P_Clear(l);
  l->scrolling = n > (size_t)LINE_CHARS;
  if (l->scrolling)
  {
    l->x = WIDTH;
    l->ms = 0;
  }
  else if (n > 0u)
  {
    calculateLEDString(l->text, LAYOUT_CENTER_X(n, 8), l->y, Font8x8);
  }
  CHANGE_FONT(BLUE);
}

void TextLine_Init(TextLine *l, int32_t y, uint8_t color)
{
  memset(l, 0, sizeof *l);
  l->y = y;
  l->color = color;
}

void TextLine_Set(TextLine *l, const char *text, uint8_t color)
{
  if (strncmp(l->text, text, TEXT_LINE_MAX) == 0 && l->color == color)
  {
    return;
  }
  strncpy(l->text, text, TEXT_LINE_MAX);
  l->text[TEXT_LINE_MAX] = '\0';
  l->color = color;
  TextLine_Redraw(l);
}

void TextLine_Tick(TextLine *l)
{
  if (!l->scrolling || ++l->ms < SCROLL_MS)
  {
    return;
  }
  l->ms = 0;
  int32_t len_px = (int32_t)strlen(l->text) * 8;
  CHANGE_FONT(l->color);
  calculateLEDString(l->text, l->x, l->y, Font8x8);
  calculateLEDString(" ", l->x + len_px, l->y, Font8x8);   /* wipe the trailing column */
  CHANGE_FONT(BLUE);
  if (--l->x < -len_px)
  {
    l->x = WIDTH;
  }
}

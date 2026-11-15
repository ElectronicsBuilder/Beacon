/*
 * ui_text_line.h - one 8 px text line on the matrix: centred when it fits
 * (16 characters on 128 px), scrolling right to left when it is longer.
 * small = 1: the tiny 3x5 font (32 characters), centred in the 8 rows.
 */
#ifndef APP_UI_TEXT_LINE_H
#define APP_UI_TEXT_LINE_H

#include <stdint.h>
#include "matrix.h"

#define TEXT_LINE_MAX 96

typedef struct {
  int32_t y;
  uint8_t color;                    /* MatrixColor */
  char    text[TEXT_LINE_MAX + 1];
  int32_t x;                        /* scroll position */
  uint8_t scrolling;
  uint8_t ms;
  uint16_t passes;                  /* completed scrolls (or 1 when static) */
  uint8_t small;                    /* 1 = tiny font (set after TextLine_Init) */
} TextLine;

void TextLine_Init(TextLine *l, int32_t y, MatrixColor color);
void TextLine_Set(TextLine *l, const char *text, MatrixColor color);   /* redraws only on change */
void TextLine_Redraw(TextLine *l);
void TextLine_Tick(TextLine *l);                                   /* every 1 ms: scrolling */

#endif

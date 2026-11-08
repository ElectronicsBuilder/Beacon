/*
 * ui_text_line.h - one 8 px text line on the matrix: centred when it fits
 * (16 characters on 128 px), scrolling right to left when it is longer.
 */
#ifndef APP_UI_TEXT_LINE_H
#define APP_UI_TEXT_LINE_H

#include <stdint.h>

#define TEXT_LINE_MAX 96

typedef struct {
  int32_t y;
  uint8_t color;                    /* CHANGE_FONT colour code */
  char    text[TEXT_LINE_MAX + 1];
  int32_t x;                        /* scroll position */
  uint8_t scrolling;
  uint8_t ms;
} TextLine;

void TextLine_Init(TextLine *l, int32_t y, uint8_t color);
void TextLine_Set(TextLine *l, const char *text, uint8_t color);   /* redraws only on change */
void TextLine_Redraw(TextLine *l);
void TextLine_Tick(TextLine *l);                                   /* every 1 ms: scrolling */

#endif

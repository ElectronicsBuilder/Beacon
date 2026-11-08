/*
 * ui_ticker.h - shows the messages now and then in a mode's text line.
 *
 * The line shows the mode's own text (date, label) for gap_s seconds, then one
 * message (both lines joined), until it has scrolled through once or for
 * show_s seconds if it fits, then the mode's text again. Only when the
 * message ticker is on ("msg ticker on") and there are messages.
 */
#ifndef APP_UI_TICKER_H
#define APP_UI_TICKER_H

#include "ui_text_line.h"

typedef struct {
  uint8_t  showing;           /* 1 = a message is on the line */
  uint8_t  color;             /* MatrixColor of that message */
  uint32_t ms;
  char     text[2 * 40 + 4];
} Ticker;

void Ticker_Reset(Ticker *t);
/* Every 1 ms with the line: returns the text to put on the line (the mode's
 * own text, or a message). *color is set to the message colour where the
 * line can show colours (not in the 32-row upper band). */
const char *Ticker_Tick(Ticker *t, const TextLine *line, const char *own_text, MatrixColor *color);

#endif

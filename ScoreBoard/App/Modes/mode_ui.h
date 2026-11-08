/*
 * mode_ui.h - drawing helpers shared by the display modes.
 */
#ifndef APP_MODE_UI_H
#define APP_MODE_UI_H

#include "matrix.h"
#include "layout.h"
#include "FONT.h"
#include "ui_text_line.h"
#include "ui_time_edit.h"
#include <string.h>

/* Colour of the text line next to the big digits. With 32 rows that line is
 * in the upper band, where only white is readable (B1/G1 wiring fault). */
#if HEIGHT == 64
#define MODE_UI_TEXT_COLOR  BLUE
#else
#define MODE_UI_TEXT_COLOR  WHITE
#endif

/* Big "HH:MM:SS" (8 characters, full width) */
static inline void ModeUi_DrawBig(const char *text, uint8_t color)
{
  CHANGE_FONT(color);
  calculateLEDStringScaled(text, LAYOUT_CENTER_X(strlen(text), LAYOUT_BIG_CHAR_W), LAYOUT_BIG_Y,
                           LAYOUT_BIG_FONT, LAYOUT_BIG_SCALE);
  CHANGE_FONT(BLUE);
}

#endif

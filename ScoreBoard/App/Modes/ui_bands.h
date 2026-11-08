/*
 * ui_bands.h - draws a screen made of 4 lines (see screen_layout.h).
 * The clock and countdown/count-up modes give the big text and the text
 * line; message lines and the ticker are handled here.
 */
#ifndef APP_UI_BANDS_H
#define APP_UI_BANDS_H

#include "screen_layout.h"
#include "matrix.h"

void Bands_Enter(ScreenId screen);               /* after the screen was cleared */
/* Every 1 ms. big: "HH:MM:SS"; big_color: CHANGE_FONT code (WHITE / RED);
 * text: date or label; ticker: 0 = no messages over the text (e.g. countdown end). */
void Bands_Tick(const char *big, uint8_t big_color, const char *text, int ticker);

#endif

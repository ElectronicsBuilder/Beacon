/*
 * font_tiny.h - 3 x 5 pixel font (digits, A-Z, a few signs; lower case is
 * shown as upper case). 4 px per character: 32 characters across the panel.
 * Drawn with the current text and background colours (matrix.h).
 */
#ifndef APP_FONT_TINY_H
#define APP_FONT_TINY_H

#include "matrix.h"

#define TINY_ADVANCE 4
#define TINY_HEIGHT  5

int32_t TinyFont_Width(const char *s);                    /* pixels */
int32_t TinyFont_Draw(const char *s, int32_t x, int32_t y);   /* returns the x after it */

#endif

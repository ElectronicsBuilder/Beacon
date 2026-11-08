/*
 * layout.h - where things go on the LED matrix, for 32 or 64 rows.
 *
 * Text lines use the 8 px font. With 32 rows there are 4 lines (y 0/8/16/24);
 * with 64 rows the same 4 lines are spread over the full height (y 4/20/36/52).
 * Big digits (Font16x24) take 3 text lines; the line under them stays free
 * for a date or a message.
 */
#ifndef APP_LAYOUT_H
#define APP_LAYOUT_H

#include "matrix.h"

#define LAYOUT_LINES        4
#if HEIGHT == 64
#define LAYOUT_LINE_Y(n)    ((n) * 16 + 4)
#define LAYOUT_BIG_Y        8     /* 24 px digits */
#define LAYOUT_BIG_SUB_Y    44    /* line under the big digits */
#else
#define LAYOUT_LINE_Y(n)    ((n) * 8)
#define LAYOUT_BIG_Y        0
#define LAYOUT_BIG_SUB_Y    24
#endif

/* x that centres 'chars' characters of a font 'char_w' pixels wide */
#define LAYOUT_CENTER_X(chars, char_w)  ((WIDTH - (int32_t)(chars) * (char_w)) / 2)

#endif

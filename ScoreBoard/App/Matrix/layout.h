/*
 * layout.h - where things go on the LED matrix, for 32 or 64 rows.
 *
 * Text lines use the 8 px font. With 32 rows there are 4 lines (y 0/8/16/24);
 * with 64 rows the same 4 lines are spread over the full height (y 4/20/36/52).
 *
 * Big digits (clock, countdown), 8 characters "HH:MM:SS" = 128 px wide:
 *  - 64 rows: Font16x24, with a free line under them.
 *  - 32 rows: the panels show image rows 0-15 and 16-31 as two separate bands
 *    (E line not wired, see matrix.h), so 24 px digits would be split. The
 *    digits are Font8x8 drawn at double size (16x16) in the lower band, which
 *    also has correct colours; the upper band holds one text line.
 */
#ifndef APP_LAYOUT_H
#define APP_LAYOUT_H

#include "matrix.h"

#define LAYOUT_LINES        4
#if HEIGHT == 64
#define LAYOUT_LINE_Y(n)    ((n) * 16 + 4)
#define LAYOUT_BIG_FONT     Font16x24
#define LAYOUT_BIG_SCALE    1
#define LAYOUT_BIG_Y        8     /* 24 px digits */
#define LAYOUT_BIG_SUB_Y    44    /* text line with the big digits */
#else
#define LAYOUT_LINE_Y(n)    ((n) * 8)
#define LAYOUT_BIG_FONT     Font8x8
#define LAYOUT_BIG_SCALE    2
#define LAYOUT_BIG_Y        16    /* lower band */
#define LAYOUT_BIG_SUB_Y    4     /* centred in the upper band */
#endif
#define LAYOUT_BIG_CHAR_W   (LAYOUT_BIG_FONT.Width * LAYOUT_BIG_SCALE)

/* x that centres 'chars' characters of a font 'char_w' pixels wide */
#define LAYOUT_CENTER_X(chars, char_w)  ((WIDTH - (int32_t)(chars) * (int32_t)(char_w)) / 2)

#endif

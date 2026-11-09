/*
 * screen_layout.h - what each of the 4 display lines shows, per screen.
 *
 * The two 64x64 panels make 4 lines (bands) of 16 rows, numbered 1..4 from
 * the top. A line of the clock, countdown or count-up screen shows one item
 * over its full height, or is split into two 8 px rows ("top/bottom"):
 *   big     HH:MM:SS, 16 px (full height only)
 *   time    HH:MM:SS, 8 px
 *   text    date (clock) or days/label (countdown, count-up)
 *   ticker  text, with messages taking turns
 *   msg     messages; full height: two-line messages use both rows,
 *           in a split line: one row (both lines joined)
 *   off     nothing
 * Each line also has a background colour (black = none).
 *
 * The "end" screen is shown after a countdown reaches zero ("cd after then
 * screen"): text = the end text, big/time = time since zero.
 *
 * With MATRIX_PANEL_ROWS 32 (E line not wired) only lines 1 and 3 light up;
 * lines 2 and 4 keep their setting for when E is wired.
 * Saved in the EEPROM (record at 0x0F40).
 */
#ifndef APP_SCREEN_LAYOUT_H
#define APP_SCREEN_LAYOUT_H

#include <stdint.h>
#include <stddef.h>

#define SCREEN_BANDS 4
#define BAND_NONE    0xFFu          /* bottom of a line that is not split */

typedef enum { BAND_OFF, BAND_BIG, BAND_TEXT, BAND_TICKER, BAND_MSG, BAND_TIME, BAND_KIND_COUNT } BandKind;
typedef enum { SCREEN_CLOCK, SCREEN_COUNTDOWN, SCREEN_COUNTUP, SCREEN_END, SCREEN_COUNT } ScreenId;

typedef struct {
  uint8_t top;                      /* BandKind (whole line when bottom is BAND_NONE) */
  uint8_t bottom;                   /* BandKind of the lower row, or BAND_NONE */
  uint8_t bg;                       /* MatrixColor of the background (MTX_C_BLACK = none) */
} BandCfg;

typedef struct {
  BandCfg band[SCREEN_BANDS];       /* top line first */
} ScreenLayout;

void Screen_Load(void);                                 /* at boot */
const ScreenLayout *Screen_Layout(ScreenId s);
int  Screen_SetBand(ScreenId s, int band, uint8_t top, uint8_t bottom);   /* band 0..3; saved; 0 = ok */
int  Screen_SetBackground(ScreenId s, int band, uint8_t color);           /* saved; 0 = ok */
uint32_t Screen_Changes(void);                          /* increments on every change */
const char *Screen_Name(ScreenId s);
int  Screen_Find(const char *name);                     /* ScreenId or -1 */
/* "big", "time/text" ... <-> top/bottom; parse returns 0 = ok */
int  Screen_ParseBand(const char *text, uint8_t *top, uint8_t *bottom);
void Screen_FormatBand(const BandCfg *b, char *text, size_t len);
int  Screen_BandVisible(int band);                      /* 0 when not wired (32 rows) */

#endif

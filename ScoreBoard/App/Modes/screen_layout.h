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
 *   now     date and time in the tiny 3x5 font, right aligned (SAT 15 NOV 14:32:05)
 *   timer   countdown (count-up on the count-up screen) with seconds, tiny
 *           font, right aligned (49D 05:12:33)
 *   info    both in one row: date and time on the left, the timer on the right
 *   bigtext the text item (date / label) in the big-digit font, 16 px (full
 *           height only); longer than ~9 characters it scrolls
 *   date    tiny date, right aligned (SUN 15 NOV 2026)
 *   clock   tiny time of day with seconds, right aligned (14:32:05)
 *   off     nothing
 * Each line also has a background colour (black = none). Each screen has
 * its own set of messages: all, none or some ids ("screen clock msgs none").
 *
 * The "end" screen is shown after a countdown reaches zero ("cd after then
 * screen"): text = the end text, big/time = time since zero.
 *
 * With MATRIX_PANEL_ROWS 32 (E line not wired) only lines 1 and 3 light up;
 * lines 2 and 4 keep their setting for when E is wired.
 * Saved in the EEPROM (record at 0x0F10).
 */
#ifndef APP_SCREEN_LAYOUT_H
#define APP_SCREEN_LAYOUT_H

#include <stdint.h>
#include <stddef.h>

#define SCREEN_BANDS 4
#define BAND_NONE    0xFFu          /* bottom of a line that is not split */

typedef enum { BAND_OFF, BAND_BIG, BAND_TEXT, BAND_TICKER, BAND_MSG, BAND_TIME,
               BAND_NOW, BAND_TIMER, BAND_INFO, BAND_DATE, BAND_CLOCK,
               BAND_BIGTEXT, BAND_KIND_COUNT } BandKind;   /* new ones at the end: stored */
typedef enum { SCREEN_CLOCK, SCREEN_COUNTDOWN, SCREEN_COUNTUP, SCREEN_END, SCREEN_COUNT } ScreenId;

typedef struct {
  uint8_t top;                      /* BandKind (whole line when bottom is BAND_NONE) */
  uint8_t bottom;                   /* BandKind of the lower row, or BAND_NONE */
  uint8_t bg;                       /* MatrixColor of the background (MTX_C_BLACK = none) */
} BandCfg;

typedef struct {
  BandCfg  band[SCREEN_BANDS];      /* top line first */
  uint32_t msg_mask;                /* messages this screen shows (bit id-1); 0 = none */
} ScreenLayout;

#define SCREEN_MSGS_ALL 0xFFFFFFFFu

/* Colours of a screen's big digits and text lines; SCREEN_COLOR_AUTO = the
 * default (big white, text green; lines 1-2 white because of the HUB75
 * fault). A chosen colour is used as it is, also in lines 1-2. */
#define SCREEN_COLOR_AUTO 0xFFu
typedef struct {
  uint8_t big;                      /* MatrixColor or SCREEN_COLOR_AUTO */
  uint8_t text;
  uint8_t sand;                     /* hourglass sand (auto = amber) */
} ScreenColors;

/* Per row of a line (row 0 = the whole line or its top half, row 1 = the
 * bottom half of a split line): text colour (SCREEN_COLOR_AUTO = the screen's
 * colours / a message's own colour; a colour overrides everything on the row)
 * and the tiny 3x5 font for text, time, ticker and msg items. */
typedef struct {
  uint8_t color;                    /* MatrixColor or SCREEN_COLOR_AUTO */
  uint8_t small;                    /* 1 = tiny font */
} RowStyle;

void Screen_Load(void);                                 /* at boot */
const RowStyle *Screen_RowStyle(ScreenId s, int band, int row);
int  Screen_SetRowColor(ScreenId s, int band, int row, uint8_t color);   /* saved; 0 = ok */
int  Screen_SetRowSmall(ScreenId s, int band, int row, uint8_t on);      /* saved; 0 = ok */
const ScreenLayout *Screen_Layout(ScreenId s);
int  Screen_SetBand(ScreenId s, int band, uint8_t top, uint8_t bottom);   /* band 0..3; saved; 0 = ok */
int  Screen_SetBackground(ScreenId s, int band, uint8_t color);           /* saved; 0 = ok */
int  Screen_SetMessages(ScreenId s, uint32_t mask);                        /* saved; 0 = ok */
const ScreenColors *Screen_Colors(ScreenId s);
int  Screen_SetColor(ScreenId s, int which, uint8_t color);                /* 0 big, 1 text, 2 sand; saved */
/* "all", "none" or "1 3 5" <-> mask; parse takes the words after "msgs" */
int  Screen_ParseMessages(int argc, char **argv, uint32_t *mask);
void Screen_FormatMessages(uint32_t mask, char *text, size_t len);
uint32_t Screen_Changes(void);                          /* increments on every change */
const char *Screen_Name(ScreenId s);
int  Screen_Find(const char *name);                     /* ScreenId or -1 */
/* "big", "time/text" ... <-> top/bottom; parse returns 0 = ok */
int  Screen_ParseBand(const char *text, uint8_t *top, uint8_t *bottom);
void Screen_FormatBand(const BandCfg *b, char *text, size_t len);
int  Screen_BandVisible(int band);                      /* 0 when not wired (32 rows) */

#endif

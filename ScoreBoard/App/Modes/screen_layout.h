/*
 * screen_layout.h - what each of the 4 display lines shows, per screen.
 *
 * The two 64x64 panels make 4 lines (bands) of 16 rows, numbered 1..4 from
 * the top. Each line of the clock, countdown and count-up screens shows one
 * of: off, big (HH:MM:SS), text (date / label), ticker (text, with messages
 * taking turns), msg (messages; two-line messages use both 8 px rows).
 *
 * With MATRIX_PANEL_ROWS 32 (E line not wired) only lines 1 and 3 light up;
 * lines 2 and 4 keep their setting for when E is wired.
 * Saved in the EEPROM (record at 0x0EE0).
 */
#ifndef APP_SCREEN_LAYOUT_H
#define APP_SCREEN_LAYOUT_H

#include <stdint.h>

#define SCREEN_BANDS 4

typedef enum { BAND_OFF, BAND_BIG, BAND_TEXT, BAND_TICKER, BAND_MSG, BAND_KIND_COUNT } BandKind;
typedef enum { SCREEN_CLOCK, SCREEN_COUNTDOWN, SCREEN_COUNTUP, SCREEN_COUNT } ScreenId;

typedef struct {
  uint8_t kind[SCREEN_BANDS];           /* BandKind per line, top first */
} ScreenLayout;

void Screen_Load(void);                                 /* at boot */
const ScreenLayout *Screen_Layout(ScreenId s);
int  Screen_SetBand(ScreenId s, int band, BandKind kind);   /* band 0..3; saved; 0 = ok */
uint32_t Screen_Changes(void);                          /* increments on every change */
const char *Screen_Name(ScreenId s);
int  Screen_Find(const char *name);                     /* ScreenId or -1 */
const char *Screen_KindName(BandKind k);
int  Screen_KindFind(const char *name);                 /* BandKind or -1 */
int  Screen_BandVisible(int band);                      /* 0 when not wired (32 rows) */

#endif

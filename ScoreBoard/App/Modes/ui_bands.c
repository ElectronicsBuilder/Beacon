/*
 * ui_bands.c - draws the 4 display lines of a screen. See ui_bands.h.
 *
 * Each line holds one item over 16 rows, or two items of 8 rows (split).
 * An item ("slot") is drawn with the line's background colour.
 */
#include "ui_bands.h"
#include "ui_text_line.h"
#include "ui_ticker.h"
#include "msg_store.h"
#include "clock.h"
#include "cmsis_os.h"
#include "layout.h"
#include "FONT.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  uint8_t  kind;             /* BandKind */
  uint8_t  full;             /* 1 = uses the whole 16-row line */
  int32_t  y;                /* top row of the slot */
  TextLine a, b;             /* text / message rows (b: line 2 of a full-height message) */
  Ticker   ticker;
  int      msg_id;
  uint32_t msg_ms;
  uint32_t msg_changes;
  char     big[12];
  uint8_t  big_color;
  char     joined[2 * MSG_LINE_MAX + 4];   /* one-row message: both lines */
} Slot;

typedef struct {
  Slot    slot[2];
  uint8_t count;             /* 0 = line not shown */
  uint8_t bg;                /* MatrixColor */
} Band;

static Band bands[SCREEN_BANDS];
static ScreenId screen;
static uint32_t layout_changes;

/* top row of a line on the image, -1 = not shown */
static int32_t P_Y(int b)
{
  if (!Screen_BandVisible(b)) return -1;
#if HEIGHT == 64
  return b * 16;
#else
  return (b == 0) ? 0 : 16;          /* lines 1 and 3 */
#endif
}

/* The upper panel half (lines 1-2) only shows white (HUB75 B1/G1 fault) */
static MatrixColor P_Color(int b, MatrixColor c)
{
  return (LAYOUT_TOP_HALF_WHITE_ONLY && b < 2) ? MTX_C_WHITE : c;
}

static void P_ClearRows(int32_t y, int rows)
{
  for (int r = 0; r < rows; r += 8)
  {
    calculateLEDString("                ", 0, y + r, Font8x8);
  }
}

/* ------------------------------------------------------------ messages */

static void P_ShowMsg(int b, Slot *s, int id)
{
  const Msg *m = MsgStore_Get(id);
  P_ClearRows(s->y, s->full ? 16 : 8);
  s->msg_id = (m != NULL) ? id : 0;
  s->msg_ms = 0;
  MatrixColor c = P_Color(b, m ? (MatrixColor)m->color : MTX_C_WHITE);
  if (m != NULL && s->full && m->line2[0])
  {
    TextLine_Init(&s->a, s->y, c);
    TextLine_Init(&s->b, s->y + 8, c);
    TextLine_Set(&s->a, m->line1, c);
    TextLine_Set(&s->b, m->line2, c);
    return;
  }
  TextLine_Init(&s->a, s->full ? s->y + 4 : s->y, c);
  TextLine_Init(&s->b, s->y + 8, c);
  if (m != NULL)
  {
    snprintf(s->joined, sizeof s->joined, m->line2[0] ? "%s  %s" : "%s", m->line1, m->line2);
    TextLine_Set(&s->a, s->joined, c);
  }
}

static void P_MsgTick(int b, Slot *s)
{
  const MsgConfig *c = MsgStore_Config();
  if (s->msg_changes != MsgStore_Changes())
  {
    s->msg_changes = MsgStore_Changes();
    if (s->msg_id == 0 || MsgStore_Get(s->msg_id) == NULL)
    {
      P_ShowMsg(b, s, MsgStore_Next());
      return;
    }
  }
  if (s->msg_id == 0 && MsgStore_PlaylistCount() > 0)
  {
    P_ShowMsg(b, s, MsgStore_Next());
    return;
  }
  TextLine_Tick(&s->a);
  TextLine_Tick(&s->b);
  s->msg_ms++;
  int done = s->msg_ms >= (uint32_t)c->show_s * 1000u &&
             (!s->a.scrolling || s->a.passes >= 1u) && (!s->b.scrolling || s->b.passes >= 1u);
  if (s->msg_id != 0 && done && MsgStore_PlaylistCount() > 1)
  {
    P_ShowMsg(b, s, MsgStore_Next());
  }
}

/* ------------------------------------------------------------ screen */

static void P_InitSlot(int b, Slot *s, uint8_t kind, int32_t y, int full)
{
  memset(s, 0, sizeof *s);
  s->kind = kind;
  s->full = (uint8_t)full;
  s->y = y;
  int32_t text_y = (full && kind != BAND_BIG) ? y + 4 : y;   /* text centred in a full line */
  TextLine_Init(&s->a, text_y, MTX_C_WHITE);
  TextLine_Init(&s->b, y + 8, MTX_C_WHITE);
  Ticker_Reset(&s->ticker);
  s->msg_changes = MsgStore_Changes();
  if (kind == BAND_MSG) P_ShowMsg(b, s, MsgStore_Next());
}

void Bands_Enter(ScreenId sc)
{
  screen = sc;
  layout_changes = Screen_Changes();
  const ScreenLayout *l = Screen_Layout(sc);
  MsgStore_SetPlaylist(l->msg_mask);            /* this screen's messages (none = 0) */
  for (int b = 0; b < SCREEN_BANDS; b++)
  {
    Band *band = &bands[b];
    const BandCfg *cfg = &l->band[b];
    int32_t y = P_Y(b);
    band->count = 0;
    if (y < 0) continue;
    band->bg = cfg->bg;
    Matrix_SetBackColor((MatrixColor)band->bg);
    P_ClearRows(y, 16);
    if (cfg->bottom == BAND_NONE)
    {
      band->count = 1;
      P_InitSlot(b, &band->slot[0], cfg->top, y, 1);
    }
    else
    {
      band->count = 2;
      P_InitSlot(b, &band->slot[0], cfg->top, y, 0);
      P_InitSlot(b, &band->slot[1], cfg->bottom, y + 8, 0);
    }
  }
  Matrix_SetBackColor(MTX_C_BLACK);
}

static void P_SlotTick(int b, Slot *s, const char *big, uint8_t big_color, const char *text, int ticker)
{
  switch (s->kind)
  {
    case BAND_BIG:
      if (strcmp(big, s->big) != 0 || big_color != s->big_color)
      {
        size_t n = strlen(big);
        int32_t adv = (n * 16u <= (size_t)WIDTH) ? 16 : 14;    /* "49D 05:12": 9 characters */
        if (n != strlen(s->big)) P_ClearRows(s->y, 16);          /* format changed (days) */
        strncpy(s->big, big, sizeof s->big - 1u);
        s->big_color = big_color;
        CHANGE_FONT(big_color);
        calculateLEDStringScaledAdv(big, (WIDTH - (int32_t)n * adv + (adv - 16)) / 2, s->y, Font8x8, 2, adv);
      }
      break;
    case BAND_TIME:
      TextLine_Set(&s->a, big, P_Color(b, big_color == RED ? MTX_C_RED : MTX_C_WHITE));
      break;
    case BAND_TEXT:
    case BAND_TICKER:
    {
      MatrixColor c = MTX_C_GREEN;
      const char *shown = text;
      if (s->kind == BAND_TICKER && ticker)
      {
        shown = Ticker_Tick(&s->ticker, &s->a, text, &c);
      }
      if (Clock_IsStale() && (osKernelGetTickCount() / 1000u) % 2u)   /* time behind: blink */
      {
        shown = "SYNC TIME";
        c = MTX_C_RED;
      }
      TextLine_Set(&s->a, shown, P_Color(b, c));
      TextLine_Tick(&s->a);
      break;
    }
    case BAND_MSG:
      P_MsgTick(b, s);
      break;
    default:
      break;
  }
}

void Bands_Tick(const char *big, uint8_t big_color, const char *text, int ticker)
{
  if (layout_changes != Screen_Changes())          /* changed from the console */
  {
    calculateClearImage();
    Bands_Enter(screen);
  }
  for (int b = 0; b < SCREEN_BANDS; b++)
  {
    Band *band = &bands[b];
    if (band->count == 0) continue;
    Matrix_SetBackColor((MatrixColor)band->bg);
    for (int i = 0; i < band->count; i++)
    {
      P_SlotTick(b, &band->slot[i], big, big_color, text, ticker);
    }
  }
  Matrix_SetBackColor(MTX_C_BLACK);
  CHANGE_FONT(BLUE);
}

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
  int8_t   hg_drawn;         /* hourglass seconds shown, -1 = none */
  char     joined[2 * MSG_LINE_MAX + 4];   /* one-row message: both lines */
} Slot;

typedef struct {
  Slot    slot[2];
  uint8_t count;             /* 0 = line not shown */
  uint8_t bg;                /* MatrixColor */
} Band;

static Band bands[SCREEN_BANDS];
static int hg_top = -1;      /* hourglass: seconds of sand in the top bulb */
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

/* ------------------------------------------------------------ hourglass */

/* 9 x 16 hourglass: outline half-width per row (cap rows 0 and 15 are full) */
static const int8_t hg_hw[16] = { 4, 4, 4, 3, 2, 1, 1, 1, 1, 1, 1, 2, 3, 4, 4, 4 };
#define HG_W 9

static void P_Px(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b)
{
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  struct color *p = &image[y][x];
  if (p->R != r || p->G != g || p->B != b) { p->R = r; p->G = g; p->B = b; matrix_dirty = 1; }
}

/* top = seconds of sand left in the top bulb (0..60): the top bulb (rows
 * 1-6) drains from its surface, the bottom bulb (rows 9-14) fills from below,
 * a grain falls through the neck every other second while sand runs. */
static void P_Hourglass(int32_t x0, int32_t y0, int top, int band)
{
  const struct color bg = background_color;
  int cells = 0;
  for (int r = 1; r <= 6; r++) cells += 2 * hg_hw[r] - 1;       /* interior of one bulb */
  int in_top = (top * cells + 30) / 60;
  int in_bottom = cells - in_top;
  uint8_t glass = (LAYOUT_TOP_HALF_WHITE_ONLY && band < 2) ? 0xC0 : 0x70;

  for (int r = 0; r < 16; r++)
  {
    int hw = hg_hw[r];
    for (int c = 0; c < HG_W; c++)
    {
      int d = c - HG_W / 2;
      if (d < 0) d = -d;
      int32_t x = x0 + c, y = y0 + r;
      if (r == 0 || r == 15)              { P_Px(x, y, glass, glass, glass); continue; }
      if (d == hw)                        { P_Px(x, y, glass, glass, glass); continue; }
      if (d > hw)                         { P_Px(x, y, bg.R, bg.G, bg.B); continue; }
      P_Px(x, y, bg.R, bg.G, bg.B);        /* interior: sand drawn below */
    }
  }
  /* sand: top bulb keeps the cells nearest the neck, bottom fills from below */
  for (int pass = 0; pass < 2; pass++)
  {
    int left = pass == 0 ? in_top : in_bottom;
    for (int i = 0; i < 6 && left > 0; i++)
    {
      int r = pass == 0 ? 6 - i : 14 - i;              /* top: rows 6..1, bottom: 14..9 */
      int hw = hg_hw[r];
      for (int d = 0; d < hw && left > 0; d++)          /* centre outwards */
      {
        for (int side = -1; side <= 1 && left > 0; side += 2)
        {
          if (d == 0 && side > 0) continue;
          P_Px(x0 + HG_W / 2 + side * d, y0 + r, 255, 190, 0);
          left--;
        }
      }
    }
  }
  if (in_top > 0 && in_bottom >= 0 && top < 60)         /* falling grain */
  {
    int r = 7 + (top % 2);
    P_Px(x0 + HG_W / 2, y0 + r, 255, 190, 0);
  }
}

void Bands_SetSeconds(int top_seconds)
{
  hg_top = (top_seconds < 0) ? -1 : (top_seconds > 60 ? 60 : top_seconds);
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
    {
      size_t n = strlen(big);
      /* days ("49D05:12"): the hourglass takes one more 14 px slot after the minutes */
      int hourglass = hg_top >= 0 && strchr(big, 'D') != NULL && n <= 8u;
      size_t slots = n + (hourglass ? 1u : 0u);
      int32_t adv = (!hourglass && slots * 16u <= (size_t)WIDTH) ? 16 : 14;
      int32_t x0 = (WIDTH - (int32_t)slots * adv + (adv - 16)) / 2;
      int redraw = strcmp(big, s->big) != 0 || big_color != s->big_color;
      if (redraw)
      {
        if (n != strlen(s->big) || hourglass != (s->hg_drawn != -2)) P_ClearRows(s->y, 16);   /* layout changed */
        strncpy(s->big, big, sizeof s->big - 1u);
        s->big_color = big_color;
        uint8_t bc = Screen_Colors(screen)->big;
        if (big_color == RED || bc == SCREEN_COLOR_AUTO) CHANGE_FONT(big_color);
        else Matrix_SetTextColor((MatrixColor)bc);
        calculateLEDStringScaledAdv(big, x0, s->y, Font8x8, 2, adv);
        s->hg_drawn = hourglass ? -1 : -2;                       /* -2: no hourglass shown */
      }
      if (hourglass && hg_top != s->hg_drawn)                    /* after the minutes */
      {
        int32_t slot = x0 + (int32_t)n * adv;
        P_Hourglass(slot + (adv - HG_W) / 2 + 1, s->y, hg_top, b);
        s->hg_drawn = (int8_t)hg_top;
      }
      break;
    }
    case BAND_TIME:
    {
      uint8_t bc = Screen_Colors(screen)->big;
      MatrixColor c = big_color == RED ? MTX_C_RED : (bc == SCREEN_COLOR_AUTO ? P_Color(b, MTX_C_WHITE) : (MatrixColor)bc);
      TextLine_Set(&s->a, big, c);
      break;
    }
    case BAND_TEXT:
    case BAND_TICKER:
    {
      uint8_t tc = Screen_Colors(screen)->text;
      MatrixColor c = (tc == SCREEN_COLOR_AUTO) ? MTX_C_GREEN : (MatrixColor)tc;
      const char *shown = text;
      if (s->kind == BAND_TICKER && ticker)
      {
        shown = Ticker_Tick(&s->ticker, &s->a, text, &c);   /* a message: its own colour */
      }
      int own = (shown == text);
      if (Clock_IsStale() && (osKernelGetTickCount() / 1000u) % 2u)   /* time behind: blink */
      {
        shown = "SYNC TIME";
        c = MTX_C_RED;
        own = 0;
      }
      TextLine_Set(&s->a, shown, (own && tc != SCREEN_COLOR_AUTO) ? c : P_Color(b, c));
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

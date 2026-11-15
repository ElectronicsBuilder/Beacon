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
#include "countdown.h"
#include "font_tiny.h"
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
  int32_t  hg_x;             /* where the hourglass goes (after the minutes) */
  char     joined[2 * MSG_LINE_MAX + 4];   /* one-row message: both lines */
  char     tiny[56];         /* now / timer / info: what is drawn */
  uint8_t  color;            /* row text colour, SCREEN_COLOR_AUTO = the usual */
  uint8_t  small;            /* tiny font for text / time / ticker / msg */
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

/* the row's own colour when one is set, else c */
#define ROW_COLOR(s, c) ((s)->color != SCREEN_COLOR_AUTO ? (MatrixColor)(s)->color : (c))

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
static void P_Hourglass(int32_t x0, int32_t y0, int top, int band, uint8_t sr, uint8_t sg, uint8_t sb)
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
          P_Px(x0 + HG_W / 2 + side * d, y0 + r, sr, sg, sb);
          left--;
        }
      }
    }
  }
  if (in_top > 0 && in_bottom >= 0 && top < 60)         /* falling grain */
  {
    int r = 7 + (top % 2);
    P_Px(x0 + HG_W / 2, y0 + r, sr, sg, sb);
  }
}

/* ------------------------------------------------------------ big digits */

/* Big digits (8x8 font at double size) with tight spacing: a digit uses font
 * columns 1-6 (12 px), a letter its real width, the colon column 3 (2 px) with
 * 3 px either side, a space is 6 px, other gaps 2 px.
 * "49D 05:12" + hourglass = ~121 px. */
#define BIG_SCALE 2
#define BIG_GAP   2
#define HG_GAP    4

/* font columns a character uses: digits 1-6 (fixed width: the countdown does
 * not jiggle), the colon 3, letters and signs their real extent */
static void P_BigCols(char c, int *first, int *last)
{
  *first = 1;
  *last = 6;
  if (c == ':') { *first = *last = 3; return; }
  if ((c >= '0' && c <= '9') || c < 32 || c > 126) return;
  const uint16_t *rows = &Font8x8.table[(c - 32) * Font8x8.Height];
  int lo = 8, hi = -1;
  for (int r = 0; r < 8; r++)
    for (int col = 0; col < 8; col++)
      if ((rows[r] >> (7 - col)) & 1u) { if (col < lo) lo = col; if (col > hi) hi = col; }
  if (hi >= lo) { *first = lo; *last = hi; }
}

static int32_t P_BigW(char c)
{
  if (c == ' ') return 6;
  int first, last;
  P_BigCols(c, &first, &last);
  return (last - first + 1) * BIG_SCALE;
}

static int32_t P_BigGap(char a, char b) { return (a == ':' || b == ':') ? 3 : BIG_GAP; }

static int32_t P_BigWidth(const char *s, int hourglass)
{
  int32_t w = 0;
  for (size_t i = 0; s[i]; i++)
  {
    if (i > 0) w += P_BigGap(s[i - 1], s[i]);
    w += P_BigW(s[i]);
  }
  return w + (hourglass ? HG_GAP + HG_W : 0);
}

/* draws s at x, y; returns the x after the last character */
static int32_t P_BigDraw(const char *s, int32_t x, int32_t y)
{
  const struct color fg = foreground_color, bg = background_color;
  for (size_t i = 0; s[i]; i++)
  {
    char c = s[i];
    if (i > 0) x += P_BigGap(s[i - 1], c);
    int first, last;
    P_BigCols(c, &first, &last);
    if (c != ' ' && c >= 32 && c <= 126)
    {
      const uint16_t *rows = &Font8x8.table[(c - 32) * Font8x8.Height];
      for (int r = 0; r < 8; r++)
      {
        for (int col = first; col <= last; col++)
        {
          const struct color *p = ((rows[r] >> (7 - col)) & 1u) ? &fg : &bg;
          for (int dy = 0; dy < BIG_SCALE; dy++)
            for (int dx = 0; dx < BIG_SCALE; dx++)
              P_Px(x + (col - first) * BIG_SCALE + dx, y + r * BIG_SCALE + dy, p->R, p->G, p->B);
        }
      }
    }
    x += P_BigW(c);
  }
  return x;
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
  MatrixColor c = ROW_COLOR(s, P_Color(b, m ? (MatrixColor)m->color : MTX_C_WHITE));
  if (m != NULL && s->full && m->line2[0])
  {
    TextLine_Init(&s->a, s->y, c);
    TextLine_Init(&s->b, s->y + 8, c);
    s->a.small = s->b.small = s->small;
    TextLine_Set(&s->a, m->line1, c);
    TextLine_Set(&s->b, m->line2, c);
    return;
  }
  TextLine_Init(&s->a, s->full ? s->y + 4 : s->y, c);
  TextLine_Init(&s->b, s->y + 8, c);
  s->a.small = s->b.small = s->small;
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

/* ------------------------------------------------------------ big text */

#define BIGTEXT_SCROLL_MS 25u         /* 1 px per step: 40 px/s */

/* the text item in the big-digit font: centred, or scrolling when too wide */
static void P_BigTextTick(int b, Slot *s, const char *text)
{
  uint8_t tc = Screen_Colors(screen)->text;
  MatrixColor c = ROW_COLOR(s, (tc == SCREEN_COLOR_AUTO) ? P_Color(b, MTX_C_GREEN) : (MatrixColor)tc);
  uint32_t now = osKernelGetTickCount();
  if (Clock_IsStale() && (now / 1000u) % 2u)          /* time behind: blink */
  {
    text = "SYNC TIME";
    c = MTX_C_RED;
  }
  int32_t w = P_BigWidth(text, 0);
  int changed = strcmp(text, s->joined) != 0 || c != (MatrixColor)s->big_color;
  if (changed)
  {
    snprintf(s->joined, sizeof s->joined, "%s", text);
    s->big_color = (uint8_t)c;
    s->hg_x = WIDTH;                                  /* scroll start: from the right edge */
    s->msg_ms = now;
  }
  else if (w <= WIDTH || now - s->msg_ms < BIGTEXT_SCROLL_MS)
  {
    return;                                           /* nothing moves */
  }
  else
  {
    s->msg_ms = now;
    if (--s->hg_x < -w) s->hg_x = WIDTH;
  }
  P_ClearRows(s->y, 16);
  Matrix_SetTextColor(c);
  P_BigDraw(text, w <= WIDTH ? (WIDTH - w) / 2 : s->hg_x, s->y);
}

/* ------------------------------------------------------------ tiny date / timer */

/* seconds: 0 = date + HH:MM, 1 = date + HH:MM:SS, 2 = date only, 3 = time only */
static void P_NowText(char *buf, size_t len, int seconds)
{
  static const char *const wd[8] = { "", "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
  static const char *const mon[13] = { "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
  if (!Clock_IsSet())
  {
    snprintf(buf, len, "NO TIME");
    return;
  }
  ClockTime t;
  Clock_Get(&t);
  const char *w = (t.weekday >= 1u && t.weekday <= 7u) ? wd[t.weekday] : "";
  const char *m = (t.month >= 1u && t.month <= 12u) ? mon[t.month] : "";
  if (seconds == 3)
    snprintf(buf, len, "%02u:%02u:%02u", t.hour, t.minute, t.second);
  else if (seconds == 2)
    snprintf(buf, len, "%s %u %s %u", w, t.day, m, t.year);
  else if (seconds)
    snprintf(buf, len, "%s %u %s %02u:%02u:%02u", w, t.day, m, t.hour, t.minute, t.second);
  else
    snprintf(buf, len, "%s %u %s %02u:%02u", w, t.day, m, t.hour, t.minute);
}

/* count-up screen: elapsed; others: remaining (or elapsed if no countdown) */
static void P_TimerText(char *buf, size_t len)
{
  uint32_t secs = 0;
  int rc = (screen == SCREEN_COUNTUP) ? Timer_Elapsed(&secs) : Timer_Remaining(&secs);
  if (rc != 0 && screen != SCREEN_COUNTUP) rc = Timer_Elapsed(&secs);
  if (rc != 0)
  {
    snprintf(buf, len, "--:--:--");
    return;
  }
  unsigned long d = secs / 86400u, h = secs / 3600u % 24u, mi = secs / 60u % 60u, se = secs % 60u;
  if (d > 0u) snprintf(buf, len, "%luD %02lu:%02lu:%02lu", d, h, mi, se);
  else        snprintf(buf, len, "%02lu:%02lu:%02lu", h, mi, se);
}

static void P_TinyTick(int b, Slot *s, uint8_t big_color)
{
  uint32_t now = osKernelGetTickCount();
  if (s->msg_ms != 0u && now - s->msg_ms < 200u) return;      /* 5 updates per second */
  s->msg_ms = now | 1u;
  char left[24] = "", right[24] = "", key[sizeof left + sizeof right + 8];
  if (s->kind == BAND_NOW)        P_NowText(right, sizeof right, 1);
  else if (s->kind == BAND_TIMER) P_TimerText(right, sizeof right);
  else if (s->kind == BAND_DATE)  P_NowText(right, sizeof right, 2);
  else if (s->kind == BAND_CLOCK) P_NowText(right, sizeof right, 3);
  else { P_NowText(left, sizeof left, 0); P_TimerText(right, sizeof right); }
  int stale = Clock_IsStale() && (now / 1000u) % 2u;          /* time behind: blink */
  if (stale && s->kind != BAND_TIMER)
  {
    snprintf(s->kind == BAND_INFO ? left : right, sizeof left, "SYNC TIME");
  }
  snprintf(key, sizeof key, "%s|%s|%d", left, right, stale);
  if (strcmp(key, s->tiny) == 0) return;
  snprintf(s->tiny, sizeof s->tiny, "%s", key);
  P_ClearRows(s->y, s->full ? 16 : 8);
  int32_t y = s->y + (s->full ? 5 : 2);
  uint8_t tc = Screen_Colors(screen)->text, bc = Screen_Colors(screen)->big;
  MatrixColor date_c = stale ? MTX_C_RED : ROW_COLOR(s, (tc == SCREEN_COLOR_AUTO ? P_Color(b, MTX_C_GREEN) : (MatrixColor)tc));
  MatrixColor timer_c = big_color == RED ? MTX_C_RED : ROW_COLOR(s, (bc == SCREEN_COLOR_AUTO ? P_Color(b, MTX_C_WHITE) : (MatrixColor)bc));
  if (left[0])
  {
    Matrix_SetTextColor(date_c);
    TinyFont_Draw(left, 1, y);
  }
  Matrix_SetTextColor(s->kind == BAND_TIMER || s->kind == BAND_INFO ? timer_c : date_c);
  TinyFont_Draw(right, WIDTH - 1 - TinyFont_Width(right), y);
}

/* ------------------------------------------------------------ screen */

static void P_InitSlot(int b, Slot *s, uint8_t kind, int32_t y, int full, const RowStyle *st)
{
  memset(s, 0, sizeof *s);
  s->kind = kind;
  s->full = (uint8_t)full;
  s->y = y;
  s->color = st->color;
  s->small = st->small;
  int32_t text_y = (full && kind != BAND_BIG) ? y + 4 : y;   /* text centred in a full line */
  TextLine_Init(&s->a, text_y, MTX_C_WHITE);
  TextLine_Init(&s->b, y + 8, MTX_C_WHITE);
  s->a.small = s->b.small = s->small;
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
      P_InitSlot(b, &band->slot[0], cfg->top, y, 1, Screen_RowStyle(sc, b, 0));
    }
    else
    {
      band->count = 2;
      P_InitSlot(b, &band->slot[0], cfg->top, y, 0, Screen_RowStyle(sc, b, 0));
      P_InitSlot(b, &band->slot[1], cfg->bottom, y + 8, 0, Screen_RowStyle(sc, b, 1));
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
      /* days ("49D 05:12"): no seconds, an hourglass after the minutes instead */
      int hourglass = hg_top >= 0 && strchr(big, 'D') != NULL;
      int32_t w = P_BigWidth(big, hourglass);
      if (w > WIDTH) { hourglass = 0; w = P_BigWidth(big, 0); }
      int32_t x0 = (WIDTH - w) / 2;
      int redraw = strcmp(big, s->big) != 0 || big_color != s->big_color;
      if (redraw)
      {
        P_ClearRows(s->y, 16);                                   /* tight layout: positions move */
        strncpy(s->big, big, sizeof s->big - 1u);
        s->big_color = big_color;
        uint8_t bc = s->color != SCREEN_COLOR_AUTO ? s->color : Screen_Colors(screen)->big;
        if (big_color == RED || bc == SCREEN_COLOR_AUTO) CHANGE_FONT(big_color);
        else Matrix_SetTextColor((MatrixColor)bc);
        s->hg_x = P_BigDraw(big, x0, s->y) + HG_GAP;
        s->hg_drawn = -1;
      }
      if (hourglass && hg_top != s->hg_drawn)                    /* after the minutes */
      {
        uint8_t sc = Screen_Colors(screen)->sand;
        uint8_t r = 255, g = 190, bl = 0;                         /* auto: amber */
        if (sc != SCREEN_COLOR_AUTO)
        {
          Matrix_SetTextColor((MatrixColor)sc);
          r = foreground_color.R; g = foreground_color.G; bl = foreground_color.B;
        }
        P_Hourglass(s->hg_x, s->y, hg_top, b, r, g, bl);
        s->hg_drawn = (int8_t)hg_top;
      }
      break;
    }
    case BAND_TIME:
    {
      uint8_t bc = Screen_Colors(screen)->big;
      MatrixColor c = big_color == RED ? MTX_C_RED : ROW_COLOR(s, (bc == SCREEN_COLOR_AUTO ? P_Color(b, MTX_C_WHITE) : (MatrixColor)bc));
      TextLine_Set(&s->a, big, c);
      TextLine_Tick(&s->a);
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
      MatrixColor shown_c = ROW_COLOR(s, (own && tc != SCREEN_COLOR_AUTO) ? c : P_Color(b, c));
      if (Clock_IsStale() && (osKernelGetTickCount() / 1000u) % 2u)   /* time behind: blink */
      {
        shown = "SYNC TIME";
        shown_c = MTX_C_RED;
      }
      TextLine_Set(&s->a, shown, shown_c);
      TextLine_Tick(&s->a);
      break;
    }
    case BAND_MSG:
      P_MsgTick(b, s);
      break;
    case BAND_BIGTEXT:
      P_BigTextTick(b, s, text);
      break;
    case BAND_NOW:
    case BAND_TIMER:
    case BAND_INFO:
    case BAND_DATE:
    case BAND_CLOCK:
      P_TinyTick(b, s, big_color);
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

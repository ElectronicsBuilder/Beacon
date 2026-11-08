/*
 * ui_bands.c - draws the 4 display lines of a screen. See ui_bands.h.
 */
#include "ui_bands.h"
#include "ui_text_line.h"
#include "ui_ticker.h"
#include "msg_store.h"
#include "layout.h"
#include "FONT.h"
#include <string.h>

typedef struct {
  TextLine a, b;             /* text: a; message: a (+ b for line 2) */
  Ticker   ticker;
  int      msg_id;           /* message shown (msg lines) */
  uint32_t msg_ms;
  uint32_t msg_changes;
  char     big[12];
  uint8_t  big_color;
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

static void P_Clear(int32_t y)
{
  Matrix_SetTextColor(MTX_C_WHITE);
  calculateLEDString("                ", 0, y, Font8x8);
  calculateLEDString("                ", 0, y + 8, Font8x8);
}

static void P_ShowMsg(int b, int id)
{
  Band *s = &bands[b];
  int32_t y = P_Y(b);
  const Msg *m = MsgStore_Get(id);
  P_Clear(y);
  s->msg_id = (m != NULL) ? id : 0;
  s->msg_ms = 0;
  if (m == NULL)
  {
    TextLine_Init(&s->a, y + 4, MTX_C_WHITE);
    TextLine_Init(&s->b, y + 8, MTX_C_WHITE);
    return;
  }
  MatrixColor c = P_Color(b, (MatrixColor)m->color);
  if (m->line2[0])
  {
    TextLine_Init(&s->a, y, c);
    TextLine_Init(&s->b, y + 8, c);
    TextLine_Set(&s->a, m->line1, c);
    TextLine_Set(&s->b, m->line2, c);
  }
  else
  {
    TextLine_Init(&s->a, y + 4, c);
    TextLine_Init(&s->b, y + 8, c);
    TextLine_Set(&s->a, m->line1, c);
  }
}

void Bands_Enter(ScreenId s)
{
  screen = s;
  layout_changes = Screen_Changes();
  const ScreenLayout *l = Screen_Layout(s);
  for (int b = 0; b < SCREEN_BANDS; b++)
  {
    Band *st = &bands[b];
    memset(st, 0, sizeof *st);
    int32_t y = P_Y(b);
    if (y < 0) continue;
    P_Clear(y);
    TextLine_Init(&st->a, y + 4, MTX_C_WHITE);
    TextLine_Init(&st->b, y + 8, MTX_C_WHITE);
    Ticker_Reset(&st->ticker);
    st->msg_changes = MsgStore_Changes();
    if (l->kind[b] == BAND_MSG) P_ShowMsg(b, MsgStore_Next());
  }
}

static void P_MsgTick(int b)
{
  Band *s = &bands[b];
  const MsgConfig *c = MsgStore_Config();
  if (s->msg_changes != MsgStore_Changes())
  {
    s->msg_changes = MsgStore_Changes();
    if (s->msg_id == 0 || MsgStore_Get(s->msg_id) == NULL)
    {
      P_ShowMsg(b, MsgStore_Next());
      return;
    }
  }
  TextLine_Tick(&s->a);
  TextLine_Tick(&s->b);
  s->msg_ms++;
  int done = s->msg_ms >= (uint32_t)c->show_s * 1000u &&
             (!s->a.scrolling || s->a.passes >= 1u) && (!s->b.scrolling || s->b.passes >= 1u);
  if (s->msg_id != 0 && done && MsgStore_Count() > 1)
  {
    P_ShowMsg(b, MsgStore_Next());
  }
}

void Bands_Tick(const char *big, uint8_t big_color, const char *text, int ticker)
{
  if (layout_changes != Screen_Changes())          /* changed from the console */
  {
    calculateClearImage();
    Bands_Enter(screen);
  }
  const ScreenLayout *l = Screen_Layout(screen);
  for (int b = 0; b < SCREEN_BANDS; b++)
  {
    Band *s = &bands[b];
    int32_t y = P_Y(b);
    if (y < 0) continue;
    switch (l->kind[b])
    {
      case BAND_BIG:
        if (strcmp(big, s->big) != 0 || big_color != s->big_color)
        {
          strncpy(s->big, big, sizeof s->big - 1u);
          s->big_color = big_color;
          CHANGE_FONT(big_color);
          calculateLEDStringScaled(big, LAYOUT_CENTER_X(strlen(big), 16), y, Font8x8, 2);
          CHANGE_FONT(BLUE);
        }
        break;
      case BAND_TEXT:
      case BAND_TICKER:
      {
        MatrixColor c = P_Color(b, MTX_C_GREEN);
        const char *shown = text;
        if (l->kind[b] == BAND_TICKER && ticker)
        {
          shown = Ticker_Tick(&s->ticker, &s->a, text, &c);
          c = P_Color(b, c);
        }
        TextLine_Set(&s->a, shown, c);
        TextLine_Tick(&s->a);
        break;
      }
      case BAND_MSG:
        P_MsgTick(b);
        break;
      default:
        break;
    }
  }
}

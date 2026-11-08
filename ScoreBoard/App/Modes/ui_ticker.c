/*
 * ui_ticker.c - messages in a mode's text line. See ui_ticker.h.
 */
#include "ui_ticker.h"
#include "msg_store.h"
#include "layout.h"
#include <stdio.h>

void Ticker_Reset(Ticker *t)
{
  t->showing = 0;
  t->ms = 0;
  t->text[0] = '\0';
}

const char *Ticker_Tick(Ticker *t, const TextLine *line, const char *own_text, MatrixColor *color)
{
  const MsgConfig *c = MsgStore_Config();
  t->ms++;
  if (!t->showing)
  {
    if (c->ticker && MsgStore_Count() > 0 && t->ms >= (uint32_t)c->gap_s * 1000u)
    {
      const Msg *m = MsgStore_Get(MsgStore_Next());
      if (m != NULL)
      {
        snprintf(t->text, sizeof t->text, m->line2[0] ? "%s  %s" : "%s", m->line1, m->line2);
        t->showing = 1;
        t->color = m->color;
        t->ms = 0;
      }
    }
  }
  else
  {
    /* done after one full scroll, or after show_s if it fits */
    int shown = t->ms > 50u && (line->scrolling ? line->passes >= 1u : t->ms >= (uint32_t)c->show_s * 1000u);
    if (shown || !c->ticker || MsgStore_Count() == 0)
    {
      t->showing = 0;
      t->ms = 0;
    }
  }
  if (t->showing)
  {
    if (!LAYOUT_MSG_L1_WHITE) *color = (MatrixColor)t->color;
    return t->text;
  }
  return own_text;
}

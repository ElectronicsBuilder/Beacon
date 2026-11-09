/*
 * mode_messages.c - plays the message list full screen.
 *
 * Each message is shown for show_s seconds, and until every line that is
 * too long has scrolled through once; then the next one (sequential or
 * random, see "msg order"). Two-line messages use both lines, one-line
 * messages the lower line.
 *
 * Keyboard: Left/Right go through all messages (in list order), Up/Down
 * change the colour of the shown one (saved), Delete removes it, Enter adds
 * a message (line 1, Enter, optional line 2, Enter; ESC cancels). After a
 * key the message stays for 30 s before playing goes on.
 */
#include "mode.h"
#include "mode_ui.h"
#include "ui_text_edit.h"
#include "msg_store.h"
#include "keys.h"
#include "log.h"
#include <string.h>

#define BROWSE_HOLD_MS 30000u      /* after a key: stay on the message this long */

static TextLine l1, l2;
static int shown_id;                 /* 0 = none */
static uint32_t shown_ms;
static uint32_t browse_ms;           /* > 0: the user is browsing, stay on this message */
static uint32_t seen_changes;
static uint8_t edit_stage;           /* 0 = playing, 1 = typing line 1, 2 = line 2 */
static char new_line1[MSG_LINE_MAX + 1];

static void P_Show(int id)
{
  const Msg *m = MsgStore_Get(id);
  calculateClearImage();
  shown_ms = 0;
  shown_id = (m != NULL) ? id : 0;
  if (m == NULL)
  {
    TextLine_Init(&l1, LAYOUT_MSG_L1_Y, MTX_C_WHITE);
    TextLine_Init(&l2, LAYOUT_MSG_L2_Y, MTX_C_WHITE);
    TextLine_Set(&l1, "NO MESSAGES", MTX_C_WHITE);
    TextLine_Set(&l2, "ENTER = ADD", MTX_C_GREEN);
    return;
  }
  MatrixColor c = (MatrixColor)m->color;
  if (m->line2[0] != '\0')
  {
    MatrixColor c1 = LAYOUT_MSG_L1_WHITE ? MTX_C_WHITE : c;
    TextLine_Init(&l1, LAYOUT_MSG_L1_Y, c1);
    TextLine_Init(&l2, LAYOUT_MSG_L2_Y, c);
    TextLine_Set(&l1, m->line1, c1);
    TextLine_Set(&l2, m->line2, c);
  }
  else
  {
    TextLine_Init(&l1, LAYOUT_MSG_SINGLE_Y, c);
    TextLine_Init(&l2, LAYOUT_MSG_L2_Y, c);
    TextLine_Set(&l1, m->line1, c);
  }
}

/* show a given message now (console "msg show"); switches to this mode */
void Messages_Show(int id)
{
  int index = Mode_Find("messages");
  if (Mode_CurrentIndex() != index) Mode_Select(index);
  MsgStore_SetCurrent(id);
  P_Show(id);
}

static void P_Enter(void)
{
  MsgStore_SetPlaylist(MSG_PLAYLIST_ALL);       /* this mode plays every message */
  edit_stage = 0;
  seen_changes = MsgStore_Changes();
  P_Show(MsgStore_Next());
}

static void P_Exit(void)
{
  edit_stage = 0;
  TextEdit_Stop();
}

static int P_Done(void)
{
  const MsgConfig *c = MsgStore_Config();
  return shown_ms >= (uint32_t)c->show_s * 1000u &&
         (!l1.scrolling || l1.passes >= 1u) && (!l2.scrolling || l2.passes >= 1u);
}

static void P_Tick(void)
{
  if (edit_stage != 0u)
  {
    TextEdit_Tick();
    return;
  }
  if (MsgStore_Changes() != seen_changes)        /* added / deleted from the console */
  {
    seen_changes = MsgStore_Changes();
    if (shown_id == 0 || MsgStore_Get(shown_id) == NULL)
    {
      P_Show(MsgStore_Next());
      return;
    }
  }
  TextLine_Tick(&l1);
  TextLine_Tick(&l2);
  shown_ms++;
  if (browse_ms > 0u) browse_ms--;
  if (shown_id != 0 && browse_ms == 0u && P_Done() && MsgStore_Count() > 1)
  {
    P_Show(MsgStore_Next());
  }
}

static void P_EditKey(uint8_t key)
{
  TimeEntryState st = TextEdit_Key(key);
  if (st == TE_CANCEL)
  {
    edit_stage = 0;
    P_Show(shown_id);
    return;
  }
  if (st != TE_DONE) return;
  if (edit_stage == 1u)
  {
    if (TextEdit_Text()[0] == '\0') return;     /* line 1 is needed */
    strcpy(new_line1, TextEdit_Text());
    edit_stage = 2;
    TextEdit_Start("LINE 2 (OPTIONAL)", "", MSG_LINE_MAX);
    return;
  }
  int id = MsgStore_Add(new_line1, TextEdit_Text(), MTX_C_WHITE);
  edit_stage = 0;
  TextEdit_Stop();
  seen_changes = MsgStore_Changes();
  if (id > 0)
  {
    LOG_I("Message %d added from the keyboard", id);
    MsgStore_SetCurrent(id);
    P_Show(id);
  }
  else
  {
    LOG_W("Message not added (%s)", id == -1 ? "list full" : "EEPROM error");
    P_Show(shown_id);
  }
}

static void P_Key(uint8_t key)
{
  if (edit_stage != 0u)
  {
    P_EditKey(key);
  }
  else if (key == KB_ENTER)
  {
    if (MsgStore_Count() >= MsgStore_Capacity())
    {
      LOG_W("Message list full (%d)", MsgStore_Capacity());
      return;
    }
    edit_stage = 1;
    TextEdit_Start("NEW MESSAGE", "", MSG_LINE_MAX);
  }
  else if (key == KB_RIGHT || key == KB_LEFT)  /* browse all messages in list order */
  {
    browse_ms = BROWSE_HOLD_MS;
    P_Show(MsgStore_Step(key == KB_RIGHT ? 1 : -1));
  }
  else if ((key == KB_UP || key == KB_DOWN) && shown_id != 0)   /* colour of the shown message */
  {
    const Msg *m = MsgStore_Get(shown_id);
    int c = m->color;
    int n = (int)MTX_C_BLACK;                     /* the text colours, black excluded */
    c = (c + (key == KB_UP ? 1 : n - 1)) % n;
    MsgStore_SetColor(shown_id, (uint8_t)c);
    seen_changes = MsgStore_Changes();
    browse_ms = BROWSE_HOLD_MS;
    LOG_I("Message %d colour: %s", shown_id, Matrix_ColorName((MatrixColor)c));
    P_Show(shown_id);
  }
  else if (key == KB_DEL && shown_id != 0)
  {
    LOG_I("Message %d deleted from the keyboard", shown_id);
    MsgStore_Delete(shown_id);
    seen_changes = MsgStore_Changes();
    P_Show(MsgStore_Next());
  }
}

const AppMode mode_messages = {
  "messages", "Messages", MODE_KIND_DISPLAY, P_Enter, P_Exit, P_Tick, P_Key
};

/*
 * cmd_msg.c - console commands for the message list.
 *
 *   msg | msg list                    list the messages in play order
 *   msg add "line 1" ["line 2"] [colour]   add (colour: white red green blue
 *                                     yellow cyan magenta; default white)
 *   msg del <id> | msg clear          delete one / all
 *   msg show <id>                     show it now (Messages mode)
 *   msg order seq|random              play order
 *   msg time <1-60>                   seconds a message that fits is shown
 *   msg ticker on|off [gap s]         also show them in the clock/countdown line
 *   msg gap <1-250>                   seconds the clock/countdown text stays
 *                                     before the ticker shows a message
 *   msg free                          free space
 */
#include "commands.h"
#include "cli.h"
#include "log.h"
#include "msg_store.h"
#include "matrix.h"
#include "mode.h"
#include <stdlib.h>
#include <string.h>

void Messages_Show(int id);          /* mode_messages.c */

static int P_Number(const char *s, long lo, long hi, long *out)
{
  char *end;
  long v = strtol(s, &end, 10);
  if (*s == '\0' || *end != '\0' || v < lo || v > hi) return -1;
  *out = v;
  return 0;
}

static void P_Free(void)
{
  int cap = MsgStore_Capacity(), n = MsgStore_Count();
  Cli_Printf("messages=%d/%d", n, cap);
  Cli_Printf("free=%d", cap - n);
  Cli_Printf("line_max=%d", MSG_LINE_MAX);
  Cli_Printf("eeprom_bytes=%lu", (unsigned long)MsgStore_EepromSize());
}

static void P_List(void)
{
  uint8_t ids[MSG_MAX];
  int n = MsgStore_Sorted(ids);
  for (int i = 0; i < n; i++)
  {
    const Msg *m = MsgStore_Get(ids[i]);
    if (m->line2[0])
      Cli_Printf("%d %s \"%s\" \"%s\"", ids[i], Matrix_ColorName((MatrixColor)m->color), m->line1, m->line2);
    else
      Cli_Printf("%d %s \"%s\"", ids[i], Matrix_ColorName((MatrixColor)m->color), m->line1);
  }
  const MsgConfig *c = MsgStore_Config();
  Cli_Printf("count=%d/%d order=%s time=%u ticker=%s gap=%u", n, MsgStore_Capacity(),
             c->order == MSG_ORDER_RANDOM ? "random" : "seq", c->show_s,
             c->ticker ? "on" : "off", c->gap_s);
}

static int P_Msg(int argc, char **argv)
{
  if (argc < 2 || strcmp(argv[1], "list") == 0)
  {
    P_List();
    return 0;
  }
  const char *sub = argv[1];
  MsgConfig c = *MsgStore_Config();
  long v;

  if (strcmp(sub, "add") == 0 && argc >= 3 && argc <= 5)
  {
    const char *line2 = "";
    int color = MTX_C_WHITE;
    for (int i = 3; i < argc; i++)
    {
      int found = Matrix_ColorFind(argv[i]);
      if (found == MTX_C_BLACK) return Cli_Error("black text would not show");
      if (i == argc - 1 && found >= 0) color = found;     /* last word may be a colour */
      else if (i == 3) line2 = argv[i];
      else return Cli_Error("unknown colour '%s'", argv[i]);
    }
    int id = MsgStore_Add(argv[2], line2, (uint8_t)color);
    if (id == -1) return Cli_Error("list full (%d messages)", MsgStore_Capacity());
    if (id == -2) return Cli_Error("lines: 1..%d printable characters", MSG_LINE_MAX);
    if (id < 0)   return Cli_Error("EEPROM write failed");
    LOG_I("Message %d added", id);
    Cli_Printf("id=%d", id);
    return 0;
  }
  if (strcmp(sub, "edit") == 0 && argc >= 4 && argc <= 6)
  {
    if (P_Number(argv[2], 1, MSG_MAX, &v) != 0 || MsgStore_Get((int)v) == NULL)
      return Cli_Error("no message %s", argv[2]);
    const char *line2 = "";
    int color = MsgStore_Get((int)v)->color;           /* colour kept unless given */
    for (int i = 4; i < argc; i++)
    {
      int found = Matrix_ColorFind(argv[i]);
      if (found == MTX_C_BLACK) return Cli_Error("black text would not show");
      if (i == argc - 1 && found >= 0) color = found;
      else if (i == 4) line2 = argv[i];
      else return Cli_Error("unknown colour '%s'", argv[i]);
    }
    if (MsgStore_Edit((int)v, argv[3], line2, (uint8_t)color) != 0)
      return Cli_Error("lines: 1..%d printable characters", MSG_LINE_MAX);
    LOG_I("Message %ld changed", v);
    return 0;
  }
  if (strcmp(sub, "colour") == 0 && argc == 4)
  {
    int c = Matrix_ColorFind(argv[3]);
    if (P_Number(argv[2], 1, MSG_MAX, &v) != 0 || MsgStore_Get((int)v) == NULL)
      return Cli_Error("no message %s", argv[2]);
    if (c < 0 || c == MTX_C_BLACK) return Cli_Error("colours: white red green blue yellow cyan magenta");
    MsgStore_SetColor((int)v, (uint8_t)c);
    return 0;
  }
  if (strcmp(sub, "del") == 0 && argc == 3)
  {
    if (P_Number(argv[2], 1, MSG_MAX, &v) != 0 || MsgStore_Delete((int)v) != 0)
      return Cli_Error("no message %s", argv[2]);
    LOG_I("Message %ld deleted", v);
    return 0;
  }
  if (strcmp(sub, "clear") == 0)
  {
    MsgStore_Clear();
    LOG_I("All messages deleted");
    return 0;
  }
  if (strcmp(sub, "show") == 0 && argc == 3)
  {
    if (P_Number(argv[2], 1, MSG_MAX, &v) != 0 || MsgStore_Get((int)v) == NULL)
      return Cli_Error("no message %s", argv[2]);
    Messages_Show((int)v);
    return 0;
  }
  if (strcmp(sub, "order") == 0 && argc == 3)
  {
    if (strcmp(argv[2], "seq") == 0) c.order = MSG_ORDER_SEQ;
    else if (strcmp(argv[2], "random") == 0) c.order = MSG_ORDER_RANDOM;
    else return Cli_Error("use msg order seq|random");
    MsgStore_SetConfig(&c);
    P_List();
    return 0;
  }
  if (strcmp(sub, "time") == 0 && argc == 3)
  {
    if (P_Number(argv[2], 1, 60, &v) != 0) return Cli_Error("seconds 1..60");
    c.show_s = (uint8_t)v;
    MsgStore_SetConfig(&c);
    P_List();
    return 0;
  }
  if (strcmp(sub, "ticker") == 0 && (argc == 3 || argc == 4))
  {
    if (strcmp(argv[2], "on") == 0) c.ticker = 1;
    else if (strcmp(argv[2], "off") == 0) c.ticker = 0;
    else return Cli_Error("use msg ticker on|off [gap seconds]");
    if (argc == 4)
    {
      if (P_Number(argv[3], 1, 250, &v) != 0) return Cli_Error("gap seconds 1..250");
      c.gap_s = (uint8_t)v;
    }
    MsgStore_SetConfig(&c);
    P_List();
    return 0;
  }
  if (strcmp(sub, "gap") == 0 && argc == 3)
  {
    if (P_Number(argv[2], 1, 250, &v) != 0) return Cli_Error("gap seconds 1..250");
    c.gap_s = (uint8_t)v;
    MsgStore_SetConfig(&c);
    P_List();
    return 0;
  }
  if (strcmp(sub, "free") == 0)
  {
    P_Free();
    return 0;
  }
  return Cli_Error("usage: msg [list|add|edit|colour|del|clear|show|order|time|gap|ticker|free] (help msg)");
}

static const CliCommand msg_commands[] = {
  { "msg", "[list|add|del|...]", "message list", P_Msg,
    "msg | msg list                list: id colour \"line 1\" \"line 2\", and the settings\n"
    "msg add \"line 1\" [\"line 2\"] [colour]   add; up to 40 characters per line\n"
    "     colour (last word): white red green blue yellow cyan magenta; default white\n"
    "     e.g. msg add \"HAPPY\" \"NEW YEAR\" yellow\n"
    "msg edit <id> \"line 1\" [\"line 2\"] [colour]   change it (same id and place)\n"
    "msg colour <id> <colour>      change only the colour\n"
    "msg del <id> | msg clear      delete one / all\n"
    "msg show <id>                 show it now (switches to Messages mode)\n"
    "msg order seq|random          play order\n"
    "msg time <1-60>               seconds a message that fits stays (long ones scroll once)\n"
    "msg gap <1-250>               seconds the screen text stays before the ticker\n"
    "                              shows a message\n"
    "msg ticker on|off [gap]       messages on the ticker lines at all\n"
    "msg free                      free places (32 messages)\n"
    "Which screens show messages: screen <s> msgs all|none|<ids>.\n"
    "Keyboard: F5 Messages mode: Left/Right browse, Up/Down colour, Enter adds,\n"
    "Del deletes; F12 menu." },
};

void CmdMsg_Register(void)
{
  Cli_Register(msg_commands, sizeof msg_commands / sizeof msg_commands[0]);
}

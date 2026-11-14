/*
 * cmd_system.c - general console commands.
 */
#include "commands.h"
#include "cli.h"
#include "log.h"
#include "version.h"
#include "clock.h"
#include "matrix.h"
#include "layout.h"
#include "settings.h"
#include "EEPROM.h"
#include "GAME.h"
#include "usb_keyboard.h"
#include "app_main.h"
#include "mode.h"
#include "keys.h"
#include "rtc_trim.h"
#include "msg_store.h"
#include "screen_layout.h"
#include "hm11.h"
#include "w25q.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern volatile uint32_t app_task_stack_free;   /* app_main.c */

static const char *P_GameState(void)
{
  switch (GAME.STATUS)
  {
    case start:                return "playing";
    case stop:                 return "finished";
    case idle:                 return "idle";
    case select_player1_mode:  return "select_player1";
    case select_player2_mode:  return "select_player2";
    case display_error_msg:    return "error";
    default:                   return "other";
  }
}

static const char *P_UsbState(void)
{
  switch (UsbKbd_Do())
  {
    case USB_HID_KEYBOARD_CONNECTED: return "keyboard";
    case USB_HID_MOUSE_CONNECTED:    return "mouse";
    case USB_HID_DEV_NOT_SUPPORTED:  return "unsupported";
    case USB_HID_DEV_CONNECTED:      return "connecting";
    default:                         return "none";
  }
}

/* info: firmware and hardware description */
static int P_Info(int argc, char **argv)
{
  (void)argc; (void)argv;
  Cli_Printf("fw=%s", FW_VERSION);
  Cli_Printf("build=%s %s", __DATE__, __TIME__);
  Cli_Printf("mcu=STM32F429VIT6");
  Cli_Printf("matrix=%ux%u", (unsigned)WIDTH, (unsigned)HEIGHT);
  Cli_Printf("top_fault=%d", LAYOUT_TOP_HALF_WHITE_ONLY);
  Cli_Printf("eeprom=%s", eeprom_ok ? "ok" : "missing");
  Cli_Printf("flash=%s", Flash_Present() ? "ok" : "none");
  return 0;
}

/* status: current state of the board */
static int P_Status(int argc, char **argv)
{
  (void)argc; (void)argv;
  char now[24] = "not set";
  if (Clock_IsSet())
  {
    ClockTime t;
    Clock_Get(&t);
    Clock_Format(&t, now, sizeof now);
  }
  Cli_Printf("time=%s", now);
  Cli_Printf("time_ok=%s", Clock_IsSet() && !Clock_IsStale() ? "yes" : "no");
  RtcTrim trim;
  if (RtcTrim_Get(&trim) == 0)
  {
    Cli_Printf("lsi_hz=%lu.%03lu", (unsigned long)(trim.lsi_mhz / 1000u), (unsigned long)(trim.lsi_mhz % 1000u));
    Cli_Printf("rtc_trim=prediv %u/%u cal %c%u", (unsigned)trim.prediv_a, (unsigned)trim.prediv_s,
               trim.calp ? 'p' : 'm', (unsigned)trim.calm);
  }
  Cli_Printf("uptime_s=%lu", (unsigned long)(osKernelGetTickCount() / 1000u));
  Cli_Printf("mode=%s", Mode_Current() ? Mode_Current()->name : "none");
  Cli_Printf("game=%s", P_GameState());
  Cli_Printf("player1=%s", PLAYER1.name);
  Cli_Printf("player2=%s", PLAYER2.name);
  Cli_Printf("score=%u:%u", (unsigned)PLAYER1.score, (unsigned)PLAYER2.score);
  Cli_Printf("brightness=%u", (unsigned)Matrix_GetBrightness());
  Cli_Printf("messages=%d/%d", MsgStore_Count(), MsgStore_Capacity());
  Cli_Printf("usb=%s", P_UsbState());
  Cli_Printf("ble=%s", Ble_StateName());
  Cli_Printf("stack_free=%lu", (unsigned long)app_task_stack_free);
  return 0;
}

/* log [error|warn|info|debug] */
static int P_Log(int argc, char **argv)
{
  static const char *names[] = { "error", "warn", "info", "debug" };
  if (argc > 1)
  {
    int level = -1;
    for (int i = 0; i < 4; i++)
    {
      if (strcmp(argv[1], names[i]) == 0) level = i;
    }
    if (level < 0) return Cli_Error("level must be error, warn, info or debug");
    log_level = (LogLevel)level;
  }
  Cli_Printf("log=%s", names[log_level]);
  return 0;
}

/* bright [5..100]: matrix brightness in percent (saved) */
static int P_Bright(int argc, char **argv)
{
  if (argc > 1)
  {
    char *end;
    long v = strtol(argv[1], &end, 10);
    if (*end != '\0' || v < MATRIX_BRIGHTNESS_MIN || v > 100)
    {
      return Cli_Error("brightness must be %d..100", MATRIX_BRIGHTNESS_MIN);
    }
    Matrix_SetBrightness((uint8_t)v);
    Settings_BrightnessChanged();
  }
  Cli_Printf("brightness=%u", (unsigned)Matrix_GetBrightness());
  return 0;
}

/* time [YYYY-MM-DD] [HH:MM[:SS]]: show or set the wall clock */
static int P_Time(int argc, char **argv)
{
  if (argc >= 2 && strcmp(argv[1], "trim") == 0)
  {
    if (argc == 3 && strcmp(argv[2], "on") == 0) RtcTrim_SetEnabled(1);
    else if (argc == 3 && strcmp(argv[2], "off") == 0) RtcTrim_SetEnabled(0);
    else if (argc != 2) return Cli_Error("use time trim on|off");
    Cli_Printf("trim=%s", RtcTrim_Enabled() ? "on" : "off");
    return 0;
  }
  if (argc > 1)
  {
    const char *date = NULL, *time = NULL;
    for (int i = 1; i < argc; i++)
    {
      if (strchr(argv[i], '-')) date = argv[i];
      else if (strchr(argv[i], ':')) time = argv[i];
      else return Cli_Error("expected YYYY-MM-DD and/or HH:MM[:SS]");
    }
    ClockTime t;
    if (Clock_Parse(date, time, &t) != 0 || Clock_Set(&t) != 0)
    {
      return Cli_Error("invalid date/time");
    }
    char text[24];
    Clock_Format(&t, text, sizeof text);
    LOG_I("Time set to %s", text);
  }
  if (!Clock_IsSet())
  {
    Cli_Printf("time=not set");
    return 0;
  }
  ClockTime now;
  char text[24];
  Clock_Get(&now);
  Clock_Format(&now, text, sizeof text);
  Cli_Printf("time=%s", text);
  return 0;
}

/* echo on|off: human mode (echo + prompt) or machine mode */
static int P_Echo(int argc, char **argv)
{
  if (argc > 1)
  {
    if (strcmp(argv[1], "on") == 0) Cli_SetMachine(0);
    else if (strcmp(argv[1], "off") == 0) Cli_SetMachine(1);
    else return Cli_Error("use echo on|off");
  }
  Cli_Printf("echo=%s", Cli_IsMachine() ? "off" : "on");
  return 0;
}

/* keys: single-key control like the 2016 serial protocol, ESC to leave */
static int P_Keys(int argc, char **argv)
{
  (void)argc; (void)argv;
  if (PingPong_IsActive())
    Cli_Printf("single-key mode: a/b player 1 +1/-1, c/d player 2 +1/-1, s start, x save, / | pause/resume; ESC to return");
  else
    Cli_Printf("single-key mode: keys go to the %s mode; ESC to return", Mode_Current()->name);
  Cli_EnterKeysMode();
  return 0;
}

/* mode [name|number]: list the modes or switch to one (saved) */
static int P_Mode(int argc, char **argv)
{
  if (argc > 1)
  {
    int index = Mode_Find(argv[1]);
    if (index < 0)
    {
      char *end;
      long n = strtol(argv[1], &end, 10);
      if (*end == '\0' && n >= 1 && n <= Mode_Count()) index = (int)n - 1;
    }
    if (index < 0) return Cli_Error("unknown mode '%s' (type mode)", argv[1]);
    Mode_Select(index);
  }
  for (int i = 0; i < Mode_Count(); i++)
  {
    const AppMode *m = Mode_Get(i);
    Cli_Printf("%c %d %-10s F%d  %s%s", i == Mode_CurrentIndex() ? '*' : ' ', i + 1, m->name,
               i + 1, m->title, m->kind == MODE_KIND_GAME ? " (game)" : "");
  }
  Cli_Printf("mode=%s", Mode_Current()->name);
  return 0;
}

/* screen [name] [...]: what each display line shows, and its background */
static void P_ScreenPrint(int s)
{
  const ScreenLayout *l = Screen_Layout((ScreenId)s);
  char text[160];
  int n = snprintf(text, sizeof text, "%s:", Screen_Name((ScreenId)s));
  for (int b = 0; b < SCREEN_BANDS; b++)
  {
    char item[24];
    Screen_FormatBand(&l->band[b], item, sizeof item);
    n += snprintf(text + n, sizeof text - (size_t)n, " %d=%s%s", b + 1, item, Screen_BandVisible(b) ? "" : "*");
    if (l->band[b].bg != MTX_C_BLACK)
    {
      n += snprintf(text + n, sizeof text - (size_t)n, "(bg %s)", Matrix_ColorName((MatrixColor)l->band[b].bg));
    }
  }
  char ids[100];
  Screen_FormatMessages(l->msg_mask, ids, sizeof ids);
  n += snprintf(text + n, sizeof text - (size_t)n, " msgs=%s", ids);
  const ScreenColors *sc = Screen_Colors((ScreenId)s);
  snprintf(text + n, sizeof text - (size_t)n, " big=%s text=%s hourglass=%s",
           sc->big == SCREEN_COLOR_AUTO ? "auto" : Matrix_ColorName((MatrixColor)sc->big),
           sc->text == SCREEN_COLOR_AUTO ? "auto" : Matrix_ColorName((MatrixColor)sc->text),
           sc->sand == SCREEN_COLOR_AUTO ? "auto" : Matrix_ColorName((MatrixColor)sc->sand));
  Cli_Printf("%s", text);
}

static int P_Screen(int argc, char **argv)
{
  static const char *items = "big, time, text, ticker, msg, off, or two of them as top/bottom (time/text)";
  if (argc >= 2)
  {
    int s = Screen_Find(argv[1]);
    uint8_t top, bottom;
    if (s < 0) return Cli_Error("screen: clock, countdown, countup or end");
    if (argc == 5 && (strcmp(argv[2], "colour") == 0 || strcmp(argv[2], "color") == 0))
    {
      int which = strcmp(argv[3], "text") == 0 ? 1 : strcmp(argv[3], "hourglass") == 0 ? 2 :
                  strcmp(argv[3], "big") == 0 ? 0 : -1;
      if (which < 0) return Cli_Error("screen <s> colour big|text|hourglass <colour|auto>");
      int c = strcmp(argv[4], "auto") == 0 ? SCREEN_COLOR_AUTO : Matrix_ColorFind(argv[4]);
      if (c < 0 || c == MTX_C_BLACK) return Cli_Error("colours: auto white red green blue yellow cyan magenta");
      Screen_SetColor((ScreenId)s, which, (uint8_t)c);
      P_ScreenPrint(s);
      return 0;
    }
    if (argc >= 3 && strcmp(argv[2], "msgs") == 0)
    {
      uint32_t mask;
      if (Screen_ParseMessages(argc - 3, argv + 3, &mask) != 0) return Cli_Error("messages: all, none or ids 1..32");
      Screen_SetMessages((ScreenId)s, mask);
      P_ScreenPrint(s);
      return 0;
    }
    if (argc == 4 || argc == 5)
    {
      char *end;
      long band = strtol(argv[2], &end, 10);
      if (*end != '\0' || band < 1 || band > SCREEN_BANDS) return Cli_Error("line 1..%d", SCREEN_BANDS);
      if (argc == 5 && strcmp(argv[3], "bg") == 0)
      {
        int c = Matrix_ColorFind(argv[4]);
        if (c < 0) return Cli_Error("colours: black white red green blue yellow cyan magenta");
        Screen_SetBackground((ScreenId)s, (int)band - 1, (uint8_t)c);
      }
      else if (argc == 4)
      {
        if (Screen_ParseBand(argv[3], &top, &bottom) != 0) return Cli_Error("show: %s", items);
        Screen_SetBand((ScreenId)s, (int)band - 1, top, bottom);
      }
      else return Cli_Error("usage: screen <name> <line> <item>  or  screen <name> <line> bg <colour>");
    }
    else if (argc == 2 + SCREEN_BANDS)
    {
      uint8_t tops[SCREEN_BANDS], bottoms[SCREEN_BANDS];
      for (int b = 0; b < SCREEN_BANDS; b++)
      {
        if (Screen_ParseBand(argv[2 + b], &tops[b], &bottoms[b]) != 0) return Cli_Error("show: %s", items);
      }
      for (int b = 0; b < SCREEN_BANDS; b++) Screen_SetBand((ScreenId)s, b, tops[b], bottoms[b]);
    }
    else if (argc != 2)
    {
      return Cli_Error("usage: screen <name> <line> <item> | <line> bg <colour> | <i1> <i2> <i3> <i4>");
    }
    P_ScreenPrint(s);
  }
  else
  {
    for (int s = 0; s < SCREEN_COUNT; s++) P_ScreenPrint(s);
  }
#if HEIGHT != 64
  Cli_Printf("(* = not lit: lines 2 and 4 need the HUB75 E line, MATRIX_PANEL_ROWS=64)");
#endif
  return 0;
}

/* key <key> ...: press keys as on the USB keyboard */
static int P_KeyCode(const char *t)
{
  static const struct { const char *name; uint8_t code; } names[] = {
    { "enter", KB_ENTER }, { "esc", KB_ESC }, { "up", KB_UP }, { "down", KB_DOWN },
    { "left", KB_LEFT }, { "right", KB_RIGHT }, { "del", KB_DEL }, { "back", KB_BACK },
    { "space", ' ' },
  };
  for (size_t i = 0; i < sizeof names / sizeof names[0]; i++)
  {
    if (strcmp(t, names[i].name) == 0) return names[i].code;
  }
  if ((t[0] == 'f' || t[0] == 'F') && t[1] >= '1' && t[1] <= '9')
  {
    int n = atoi(t + 1);
    if (n >= 1 && n <= 12) return KB_F1 + n - 1;
  }
  return -1;
}

static int P_Key(int argc, char **argv)
{
  if (argc < 2) return Cli_Error("usage: key <key> ... (help key)");
  for (int i = 1; i < argc; i++)
  {
    int code = P_KeyCode(argv[i]);
    if (code >= 0)
    {
      Mode_Key((uint8_t)code);
    }
    else
    {
      for (const char *c = argv[i]; *c; c++) Mode_Key((uint8_t)*c);   /* plain text */
    }
  }
  return 0;
}

/* test: full display test */
static int P_Test(int argc, char **argv)
{
  (void)argc; (void)argv;
  Mode_StartTest();
  Cli_Printf("display test running (about 30 s, any key stops it)");
  return 0;
}

/* reboot */
static int P_Reboot(int argc, char **argv)
{
  (void)argc; (void)argv;
  Cli_Printf("OK");
  osDelay(20);            /* let the UART finish */
  NVIC_SystemReset();
  return 0;
}

static const CliCommand system_commands[] = {
  { "info", "", "firmware version and hardware", P_Info,
    "info    firmware version, build date, MCU, matrix size, EEPROM state" },
  { "status", "", "time, mode, game, messages, USB ...", P_Status,
    "status  date/time, RTC trim (lsi_hz, rtc_trim), uptime, mode, game state,\n"
    "        players, score, brightness, messages, USB keyboard, free stack" },
  { "time", "[date] [time] | trim", "show or set the clock", P_Time,
    "time                        show the date and time\n"
    "time 2026-11-09 16:52[:30]  set date and time (24-hour clock)\n"
    "time 16:52                  set the time only (seconds = 0)\n"
    "time 2026-12-24             set the date only\n"
    "time trim on|off            RTC accuracy: on = measured against the crystal\n"
    "                            (default), off = CubeMX default (~2.5% slow here)\n"
    "Keyboard: Clock mode (F2), T or Enter.\n"
    "Note: with main power off (VBAT only) the date/time is kept but does not count." },
  { "bright", "[5-100]", "matrix brightness (saved)", P_Bright,
    "bright       show the brightness\n"
    "bright 40    set it, 5..100 % (saved)\n"
    "Keyboard: + / -, or F12 menu." },
  { "log", "[level]", "log level", P_Log,
    "log                          show the level\n"
    "log error|warn|info|debug    show messages up to this level (default info)" },
  { "echo", "[on|off]", "typing mode / machine mode", P_Echo,
    "echo off    machine mode for scripts and the GUI: no echo, no prompt,\n"
    "            every reply ends with OK or ERR <reason>\n"
    "echo on     typing mode (default)" },
  { "mode", "[name|number]", "display mode (saved)", P_Mode,
    "mode            list the modes (* = current)\n"
    "mode clock      switch by name, or by number: mode 2\n"
    "Modes: 1 pingpong, 2 clock, 3 countdown, 4 countup, 5 messages.\n"
    "The mode is saved and comes back after a reboot. Keyboard: F1..F5." },
  { "screen", "[name] [...]", "what each display line shows", P_Screen,
    "screen                            show all layouts (* = line not lit with 32 rows)\n"
    "screen <s> <line> <item>          s: clock countdown countup end, line: 1..4\n"
    "   items: big (HH:MM:SS 16 px)  time (HH:MM:SS 8 px)  text (date / label /\n"
    "          end text)  ticker (text, messages take turns)  msg (messages)  off\n"
    "   top/bottom splits a line in two 8 px rows: screen clock 3 time/text\n"
    "screen <s> <i1> <i2> <i3> <i4>     all 4 lines: screen countdown ticker msg big msg\n"
    "screen <s> <line> bg <colour>      background: black white red green blue yellow\n"
    "                                  cyan magenta (lines 1-2: blue stays on, HW fault)\n"
    "screen <s> msgs all|none|<ids>     messages on this screen: screen clock msgs none\n"
    "screen <s> colour big|text|hourglass <c>   big digits / text line / hourglass sand:\n"
    "                                  auto (default), white red green blue yellow cyan magenta\n"
    "The end screen is shown after a countdown (cd after then screen).\n"
    "Lines 2 and 4 need the HUB75 E line (MATRIX_PANEL_ROWS=64)." },
  { "key", "<key> ...", "press USB keyboard keys", P_Key,
    "key <key> ...    press keys as on the USB keyboard, e.g.\n"
    "                 key f3   key enter   key \"NEW YEAR\" enter   key up up enter\n"
    "names: enter esc up down left right del back space f1..f12; other words\n"
    "are typed letter by letter (use quotes for spaces)." },
  { "keys", "", "single-key control (ESC back)", P_Keys,
    "keys    every key goes to the display, like the 2016 serial remote.\n"
    "        Ping-pong: a/b player 1 +1/-1, c/d player 2 +1/-1, s start, x save,\n"
    "        / | pause/resume. Other modes: the keys of that mode (help keys).\n"
    "        ESC returns to the console." },
  { "test", "", "full display test (~30 s)", P_Test,
    "test    red, green, blue, white; the 4 lines labelled; colour bars; gradient;\n"
    "        checkerboard; a line over every row, then every column. Then the\n"
    "        shown mode comes back. Any key (or key esc) stops it." },
  { "reboot", "", "restart the board", P_Reboot,
    "reboot  restart (settings, messages, countdown and time are kept)" },
};

void CmdSystem_Register(void)
{
  Cli_Register(system_commands, sizeof system_commands / sizeof system_commands[0]);
}

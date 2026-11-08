/*
 * cmd_system.c - general console commands.
 */
#include "commands.h"
#include "cli.h"
#include "log.h"
#include "version.h"
#include "clock.h"
#include "matrix.h"
#include "settings.h"
#include "EEPROM.h"
#include "GAME.h"
#include "usb_keyboard.h"
#include "app_main.h"
#include "mode.h"
#include "rtc_trim.h"
#include "msg_store.h"
#include "screen_layout.h"
#include "cmsis_os.h"
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
  Cli_Printf("eeprom=%s", eeprom_ok ? "ok" : "missing");
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
    Cli_Printf("single-key mode: a/b k/m score, s start, / | pause/resume; ESC to return");
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

/* screen [name] [line kind | kind kind kind kind]: what each display line shows */
static void P_ScreenPrint(int s)
{
  const ScreenLayout *l = Screen_Layout((ScreenId)s);
  char text[96];
  int n = snprintf(text, sizeof text, "%s:", Screen_Name((ScreenId)s));
  for (int b = 0; b < SCREEN_BANDS; b++)
  {
    n += snprintf(text + n, sizeof text - (size_t)n, " %d=%s%s", b + 1,
                  Screen_KindName((BandKind)l->kind[b]), Screen_BandVisible(b) ? "" : "*");
  }
  Cli_Printf("%s", text);
}

static int P_Screen(int argc, char **argv)
{
  if (argc >= 2)
  {
    int s = Screen_Find(argv[1]);
    if (s < 0) return Cli_Error("screen: clock, countdown or countup");
    if (argc == 4)
    {
      char *end;
      long band = strtol(argv[2], &end, 10);
      int kind = Screen_KindFind(argv[3]);
      if (*end != '\0' || band < 1 || band > SCREEN_BANDS) return Cli_Error("line 1..%d", SCREEN_BANDS);
      if (kind < 0) return Cli_Error("show: off, big, text, ticker or msg");
      Screen_SetBand((ScreenId)s, (int)band - 1, (BandKind)kind);
    }
    else if (argc == 2 + SCREEN_BANDS)
    {
      int kinds[SCREEN_BANDS];
      for (int b = 0; b < SCREEN_BANDS; b++)
      {
        kinds[b] = Screen_KindFind(argv[2 + b]);
        if (kinds[b] < 0) return Cli_Error("show: off, big, text, ticker or msg");
      }
      for (int b = 0; b < SCREEN_BANDS; b++) Screen_SetBand((ScreenId)s, b, (BandKind)kinds[b]);
    }
    else if (argc != 2)
    {
      return Cli_Error("usage: screen <name> <line> <kind>  or  screen <name> <k1> <k2> <k3> <k4>");
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
  { "info",   "",                         "firmware and hardware information",            P_Info },
  { "status", "",                         "time, game, players, score, brightness, USB",  P_Status },
  { "time",   "[YYYY-MM-DD] [HH:MM[:SS]]", "show or set the date/time",                   P_Time },
  { "bright", "[5-100]",                  "show or set the matrix brightness (saved)",   P_Bright },
  { "log",    "[error|warn|info|debug]",  "show or set the log level",                    P_Log },
  { "echo",   "[on|off]",                 "off = machine mode (no echo/prompt)",          P_Echo },
  { "mode",   "[name|number]",            "list the display modes or switch (saved)",    P_Mode },
  { "screen", "[clock|countdown|countup] [<line 1-4> <kind> | <k1> <k2> <k3> <k4>]",
               "what each display line shows: off big text ticker msg", P_Screen },
  { "keys",   "",                         "single-key control of the mode (ESC to return)", P_Keys },
  { "reboot", "",                         "restart the board",                            P_Reboot },
};

void CmdSystem_Register(void)
{
  Cli_Register(system_commands, sizeof system_commands / sizeof system_commands[0]);
}

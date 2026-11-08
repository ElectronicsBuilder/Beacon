/*
 * cmd_timer.c - console commands for the countdown and count-up.
 *
 *   cd                               show the countdown
 *   cd set [YYYY-MM-DD] HH:MM[:SS]   count down to a date/time (today if no date)
 *   cd in 1h30m                      count down for a duration (d h m s)
 *   cd label "text" | cd end "text"  text while counting / at zero ("" = none)
 *   cd clear
 *   cu                               show the count-up
 *   cu start [YYYY-MM-DD] [HH:MM[:SS]]   count up from now or a date/time
 *   cu stop | cu resume | cu label "text" | cu clear
 * Setting a countdown or count-up also shows it (switches the mode).
 */
#include "commands.h"
#include "cli.h"
#include "log.h"
#include "countdown.h"
#include "mode.h"
#include "clock.h"
#include "clock_calc.h"
#include <string.h>

static void P_PrintTime(const char *name, uint32_t secs)
{
  char text[24] = "none";
  if (secs != 0u)
  {
    ClockTime t;
    ClockCalc_FromSeconds(secs, &t);
    Clock_Format(&t, text, sizeof text);
  }
  Cli_Printf("%s=%s", name, text);
}

static void P_PrintDuration(const char *name, int ok, uint32_t secs)
{
  if (ok != 0)
  {
    Cli_Printf("%s=none", name);
    return;
  }
  char hms[12];
  uint32_t days = ClockCalc_FormatDuration(secs, hms, sizeof hms);
  Cli_Printf("%s=%lud %s", name, (unsigned long)days, hms);
}

static void P_Show(int index)
{
  if (Mode_CurrentIndex() != index) Mode_Select(index);
}

/* date and/or time arguments -> seconds since 2000 (missing parts from now) */
static int P_ParseWhen(int argc, char **argv, uint32_t *secs)
{
  const char *date = NULL, *time = NULL;
  for (int i = 0; i < argc; i++)
  {
    if (strchr(argv[i], '-')) date = argv[i];
    else if (strchr(argv[i], ':')) time = argv[i];
    else return Cli_Error("expected YYYY-MM-DD and/or HH:MM[:SS]");
  }
  ClockTime t;
  if (Clock_Parse(date, time, &t) != 0) return Cli_Error("invalid date/time");
  if (date == NULL && time != NULL && strlen(time) <= 5) t.second = 0;
  *secs = ClockCalc_ToSeconds(&t);
  return 0;
}

static int P_Cd(int argc, char **argv)
{
  const TimerConfig *c = Timer_Config();
  if (argc >= 2)
  {
    uint32_t secs;
    if (!Clock_IsSet() && (strcmp(argv[1], "set") == 0 || strcmp(argv[1], "in") == 0))
      return Cli_Error("set the clock first (time ...)");
    if (strcmp(argv[1], "set") == 0 && argc >= 3)
    {
      if (P_ParseWhen(argc - 2, argv + 2, &secs) != 0) return -1;
      Timer_SetCountdown(secs);
      P_Show(Mode_Find("countdown"));
    }
    else if (strcmp(argv[1], "in") == 0 && argc == 3)
    {
      uint32_t now, dur;
      if (ClockCalc_ParseDuration(argv[2], &dur) != 0) return Cli_Error("duration like 1h30m, 2d, 45s");
      ClockTime t;
      Clock_Get(&t);
      now = ClockCalc_ToSeconds(&t);
      Timer_SetCountdown(now + dur);
      P_Show(Mode_Find("countdown"));
    }
    else if (strcmp(argv[1], "label") == 0 && argc == 3) Timer_SetCountdownLabel(argv[2]);
    else if (strcmp(argv[1], "end") == 0 && argc == 3)   Timer_SetCountdownEnd(argv[2]);
    else if (strcmp(argv[1], "clear") == 0)
    {
      Timer_SetCountdown(0);
      Timer_SetCountdownLabel("");
      Timer_SetCountdownEnd("");
    }
    else return Cli_Error("usage: cd [set <date/time> | in <duration> | label <text> | end <text> | clear]");
    if (strcmp(argv[1], "set") == 0 || strcmp(argv[1], "in") == 0)
    {
      char text[24] = "";
      ClockTime t;
      ClockCalc_FromSeconds(c->cd_target, &t);
      Clock_Format(&t, text, sizeof text);
      LOG_I("Countdown to %s", text);
    }
  }
  uint32_t secs;
  P_PrintTime("target", c->cd_target);
  P_PrintDuration("remaining", Timer_Remaining(&secs), secs);
  Cli_Printf("label=%s", c->cd_label);
  Cli_Printf("end=%s", c->cd_end);
  return 0;
}

static int P_Cu(int argc, char **argv)
{
  const TimerConfig *c = Timer_Config();
  if (argc >= 2)
  {
    if (!Clock_IsSet() && strcmp(argv[1], "label") != 0 && strcmp(argv[1], "clear") != 0)
      return Cli_Error("set the clock first (time ...)");
    if (strcmp(argv[1], "start") == 0)
    {
      uint32_t secs;
      if (argc == 2)
      {
        ClockTime t;
        Clock_Get(&t);
        secs = ClockCalc_ToSeconds(&t);
      }
      else if (P_ParseWhen(argc - 2, argv + 2, &secs) != 0) return -1;
      Timer_StartCountup(secs);
      LOG_I("Count-up started");
      P_Show(Mode_Find("countup"));
    }
    else if (strcmp(argv[1], "stop") == 0)                Timer_StopCountup();
    else if (strcmp(argv[1], "resume") == 0)              Timer_ResumeCountup();
    else if (strcmp(argv[1], "label") == 0 && argc == 3)  Timer_SetCountupLabel(argv[2]);
    else if (strcmp(argv[1], "clear") == 0)
    {
      Timer_StartCountup(0);
      Timer_SetCountupLabel("");
    }
    else return Cli_Error("usage: cu [start [date/time] | stop | resume | label <text> | clear]");
  }
  uint32_t secs;
  P_PrintTime("start", c->cu_start);
  P_PrintDuration("elapsed", Timer_Elapsed(&secs), secs);
  Cli_Printf("state=%s", c->cu_start == 0u ? "none" : (c->cu_stopped_at ? "stopped" : "running"));
  Cli_Printf("label=%s", c->cu_label);
  return 0;
}

static const CliCommand timer_commands[] = {
  { "cd", "[set <date/time> | in <1h30m> | label <text> | end <text> | clear]",
          "countdown to a date/time (shows it)", P_Cd },
  { "cu", "[start [date/time] | stop | resume | label <text> | clear]",
          "count up from now or a date/time (shows it)", P_Cu },
};

void CmdTimer_Register(void)
{
  Cli_Register(timer_commands, sizeof timer_commands / sizeof timer_commands[0]);
}

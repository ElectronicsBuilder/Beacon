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
 *   cd after                         show what happens at zero
 *   cd after scene <fireworks|stars|confetti|snow|rainbow|none> [seconds | inf]
 *   cd dismiss                       stop the scene / messages at zero (keyboard C or Del)
 *   cd after then hold|screen        after the scene: 00:00:00 + end text, or the
 *                                    "end" screen (layout: screen end ...)
 *   cd after msgs all|none|<id> ...  messages on the end screen (= screen end msgs)
 * Setting a countdown or count-up also shows it (switches the mode).
 */
#include "commands.h"
#include "cli.h"
#include "log.h"
#include "countdown.h"
#include "mode.h"
#include "clock.h"
#include "clock_calc.h"
#include "screen_layout.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
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

static void P_PrintAfter(void)
{
  const EndPlan *p = Timer_EndPlan();
  char ids[100];
  Screen_FormatMessages(Screen_Layout(SCREEN_END)->msg_mask, ids, sizeof ids);
  if (p->scene == SCENE_NONE)
    Cli_Printf("after=no scene, then %s", EndPlan_ThenName((EndThen)p->then));
  else if (p->scene_s == 0u)
    Cli_Printf("after=%s infinite", EndPlan_SceneName((SceneId)p->scene));
  else
    Cli_Printf("after=%s %us, then %s", EndPlan_SceneName((SceneId)p->scene), (unsigned)p->scene_s,
               EndPlan_ThenName((EndThen)p->then));
  Cli_Printf("end_msgs=%s", ids);
}

static int P_After(int argc, char **argv)
{
  EndPlan p = *Timer_EndPlan();
  if (argc >= 4 && strcmp(argv[2], "scene") == 0 && argc <= 5)
  {
    int s = EndPlan_SceneFind(argv[3]);
    if (s < 0) return Cli_Error("scenes: fireworks stars confetti snow rainbow none");
    p.scene = (uint8_t)s;
    if (argc == 5)
    {
      char *end;
      long v = (strcmp(argv[4], "inf") == 0 || strcmp(argv[4], "infinite") == 0) ? 0 : strtol(argv[4], &end, 10);
      if (v != 0 && (*end != '\0' || v < 0 || v > 65535)) return Cli_Error("seconds 1..65535, or inf");
      if (v == 0 && strcmp(argv[4], "0") != 0 && strncmp(argv[4], "inf", 3) != 0) return Cli_Error("seconds 1..65535, or inf");
      p.scene_s = (uint16_t)v;
    }
  }
  else if (argc == 4 && strcmp(argv[2], "then") == 0)
  {
    int t = EndPlan_ThenFind(argv[3]);
    if (t < 0) return Cli_Error("then: hold or screen");
    p.then = (uint8_t)t;
  }
  else if (argc >= 4 && strcmp(argv[2], "msgs") == 0)     /* = screen end msgs ... */
  {
    uint32_t mask;
    if (Screen_ParseMessages(argc - 3, argv + 3, &mask) != 0) return Cli_Error("messages: all, none or ids 1..32");
    Screen_SetMessages(SCREEN_END, mask);
    P_PrintAfter();
    return 0;
  }
  else if (argc != 2)
  {
    return Cli_Error("usage: cd after [scene <name> [s] | then hold|screen | msgs all|<id>...]");
  }
  if (argc != 2) Timer_SetEndPlan(&p);
  P_PrintAfter();
  return 0;
}

static int P_Cd(int argc, char **argv)
{
  if (argc >= 2 && strcmp(argv[1], "after") == 0) return P_After(argc, argv);
  if (argc == 2 && strcmp(argv[1], "dismiss") == 0)
  {
    if (Timer_DismissEnd() != 0) return Cli_Error("the countdown is not at zero");
    LOG_I("Countdown end show cleared");
    return 0;
  }
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
    else if (strcmp(argv[1], "days") == 0 && argc == 3 && (!strcmp(argv[2], "on") || !strcmp(argv[2], "off")))
      Settings_SetDays(0, strcmp(argv[2], "on") == 0);
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
  Cli_Printf("days=%s", Settings_GetDays(0) ? "on" : "off");
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
    else if (strcmp(argv[1], "days") == 0 && argc == 3 && (!strcmp(argv[2], "on") || !strcmp(argv[2], "off")))
      Settings_SetDays(1, strcmp(argv[2], "on") == 0);
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
  Cli_Printf("days=%s", Settings_GetDays(1) ? "on" : "off");
  return 0;
}

static const CliCommand timer_commands[] = {
  { "cd", "[set|in|after|...]", "countdown", P_Cd,
    "cd                              show target, remaining, label, end text\n"
    "cd set [YYYY-MM-DD] HH:MM[:SS]  count down to a date/time (today if no date)\n"
    "cd in 1h30m                     count down for a duration: d h m s, e.g. 2d, 45s,\n"
    "                                90 (= minutes)\n"
    "cd label \"NEW YEAR\"             text while counting (\"\" = none)\n"
    "cd days on|off                  also the days before the label (49D NEW YEAR);\n"
    "                                the big digits show 49D 05:12 anyway\n"
    "cd end \"HAPPY NEW YEAR\"         text at zero (\"\" = none)\n"
    "cd after                        what happens at zero:\n"
    "cd after scene <name> [s|inf]   fireworks stars confetti snow rainbow none;\n"
    "                                s = seconds, inf = while the countdown is shown\n"
    "cd after then hold|screen       after the scene: 00:00:00 + end text, or the\n"
    "                                end screen (screen end ...)\n"
    "cd after msgs all|none|<ids>    messages on the end screen\n"
    "cd dismiss                      at zero: stop the scene and messages (keyboard C/Del)\n"
    "cd clear                        remove the countdown\n"
    "Setting it switches to Countdown mode. Keyboard (F3): Enter = setup (end time,\n"
    "label, end text), T time, L label, E end text, C or Del clears the end show;\n"
    "F12 menu: end scene, scene time, after scene." },
  { "cu", "[start|stop|...]", "count-up", P_Cu,
    "cu                                  show start, elapsed, state, label\n"
    "cu start                            count up from now\n"
    "cu start [YYYY-MM-DD] [HH:MM[:SS]]  count up from a date/time (a future start\n"
    "                                    shows STARTS hh:mm)\n"
    "cu stop | cu resume                 pause / continue from the shown time\n"
    "cu label \"SHIFT\"                    text with the time\n"
    "cu days on|off                      days before the label, or label only\n"
    "cu clear                            remove the count-up\n"
    "Starting it switches to Count-up mode. Keyboard (F4): Enter = setup (start,\n"
    "label), T start, L label, S stop/resume, R restart." },
};

void CmdTimer_Register(void)
{
  Cli_Register(timer_commands, sizeof timer_commands / sizeof timer_commands[0]);
}

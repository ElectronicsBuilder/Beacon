/*
 * mode_countdown.c - countdown and count-up modes. See countdown.h.
 *
 * Keyboard: Enter = guided setup (countdown: end time, label, end text;
 * count-up: start time, label), T = time only, L = label, E = end text,
 * C / Del = clear the end show (countdown); count-up: S stop/resume, R restart.
 */
#include "countdown.h"
#include "mode.h"
#include "mode_ui.h"
#include "ui_bands.h"
#include "ui_text_edit.h"
#include "scenes.h"
#include "msg_store.h"
#include "clock.h"
#include "clock_calc.h"
#include "ee_record.h"
#include "keys.h"
#include "settings.h"
#include "log.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

#define POLL_MS        100u
#define SAVE_DELAY_MS  1000u

/* ------------------------------------------------------------ settings */

static TimerConfig cfg;
static EndPlan plan = END_PLAN_DEFAULT;
static uint8_t save_pending;
static uint8_t plan_save_pending;
static uint32_t save_at_ms;

static void P_Changed(void)
{
  save_pending = 1;
  save_at_ms = osKernelGetTickCount() + SAVE_DELAY_MS;
}

static void P_CopyText(char *dst, const char *src)
{
  strncpy(dst, src, TIMER_TEXT_MAX);
  dst[TIMER_TEXT_MAX] = '\0';
}

void Timer_Load(void)
{
  if (EeRecord_Load(EE_RECORD_TIMER_ADDR, &cfg, sizeof cfg) != 0)
  {
    memset(&cfg, 0, sizeof cfg);
  }
  cfg.cd_label[TIMER_TEXT_MAX] = cfg.cd_end[TIMER_TEXT_MAX] = cfg.cu_label[TIMER_TEXT_MAX] = '\0';
  EndPlan p;
  if (EeRecord_Load(EE_RECORD_ENDPLAN_ADDR, &p, sizeof p) == 0 && EndPlan_Valid(&p))
  {
    plan = p;
  }
}

const EndPlan *Timer_EndPlan(void) { return &plan; }

void Timer_SetEndPlan(const EndPlan *p)
{
  if (!EndPlan_Valid(p)) return;
  plan = *p;
  plan_save_pending = 1;
  P_Changed();
}

void Timer_Poll(void)
{
  if (save_pending && (int32_t)(osKernelGetTickCount() - save_at_ms) >= 0)
  {
    save_pending = 0;
    if (plan_save_pending)
    {
      plan_save_pending = 0;
      if (EeRecord_Save(EE_RECORD_ENDPLAN_ADDR, &plan, sizeof plan) != 0)
      {
        LOG_E("Countdown end plan: EEPROM write failed");
      }
    }
    if (EeRecord_Save(EE_RECORD_TIMER_ADDR, &cfg, sizeof cfg) != 0)
    {
      LOG_E("Countdown settings: EEPROM write failed");
    }
  }
}

const TimerConfig *Timer_Config(void)        { return &cfg; }
void Timer_SetCountdown(uint32_t target)     { cfg.cd_target = target; P_Changed(); }
void Timer_SetCountdownLabel(const char *s)  { P_CopyText(cfg.cd_label, s); P_Changed(); }
void Timer_SetCountdownEnd(const char *s)    { P_CopyText(cfg.cd_end, s); P_Changed(); }
void Timer_SetCountupLabel(const char *s)    { P_CopyText(cfg.cu_label, s); P_Changed(); }

void Timer_StartCountup(uint32_t start)
{
  cfg.cu_start = start;
  cfg.cu_stopped_at = 0;
  P_Changed();
}

static int P_Now(uint32_t *now)
{
  if (!Clock_IsSet()) return -1;
  ClockTime t;
  Clock_Get(&t);
  *now = ClockCalc_ToSeconds(&t);
  return 0;
}

void Timer_StopCountup(void)
{
  uint32_t now;
  if (cfg.cu_start != 0u && cfg.cu_stopped_at == 0u && P_Now(&now) == 0)
  {
    cfg.cu_stopped_at = now;
    P_Changed();
  }
}

void Timer_ResumeCountup(void)
{
  uint32_t now;
  if (cfg.cu_start != 0u && cfg.cu_stopped_at != 0u && P_Now(&now) == 0)
  {
    cfg.cu_start += now - cfg.cu_stopped_at;    /* continue from the shown time */
    cfg.cu_stopped_at = 0;
    P_Changed();
  }
}

int Timer_Remaining(uint32_t *secs)
{
  uint32_t now;
  if (cfg.cd_target == 0u || P_Now(&now) != 0) return -1;
  *secs = (cfg.cd_target > now) ? cfg.cd_target - now : 0u;
  return 0;
}

int Timer_Elapsed(uint32_t *secs)
{
  uint32_t now;
  if (cfg.cu_start == 0u || P_Now(&now) != 0) return -1;
  if (cfg.cu_stopped_at != 0u) now = cfg.cu_stopped_at;
  *secs = (now > cfg.cu_start) ? now - cfg.cu_start : 0u;
  return 0;
}

/* ------------------------------------------------------------ display */

static uint32_t poll_count;
static char big[12];
static char own_text[TIMER_TEXT_MAX + 16];   /* days / label / end text */
static uint8_t big_color = WHITE;
static uint8_t at_zero;                 /* countdown ended: no messages over the end text */
static uint8_t countup;                 /* which of the two modes is active */
static int8_t was_running = -1;         /* countdown: for the "finished" log */

/* what the countdown screen shows; after zero it follows the end plan */
typedef enum { VIEW_NONE, VIEW_NORMAL, VIEW_SCENE, VIEW_END } View;
static View view;
static char since_big[12];              /* end screen: time since zero */
static char end_text[TIMER_TEXT_MAX + 1];
static uint32_t dismissed_target;       /* end show stopped for this target (C / Del / cd dismiss) */

/* keyboard editing (see P_Key) */
typedef enum { ED_NONE, ED_TIME, ED_LABEL, ED_END } EditStep;
static EditStep editing;
static uint8_t guided;                  /* 1 = Enter setup: go on to the next step */

static void P_Update(void)
{
  char *text = own_text;
  size_t text_len = sizeof own_text;
  uint32_t secs;
  const char *label = countup ? cfg.cu_label : cfg.cd_label;

  big_color = WHITE;
  if (!Clock_IsSet())
  {
    snprintf(big, sizeof big, "--:--:--");
    snprintf(text, text_len, "SET THE CLOCK");
  }
  else if ((countup ? Timer_Elapsed(&secs) : Timer_Remaining(&secs)) != 0)
  {
    snprintf(big, sizeof big, "--:--:--");
    snprintf(text, text_len, "PRESS T TO SET");
  }
  else
  {
    uint32_t days = ClockCalc_FormatBig(secs, big, sizeof big);   /* "49D 05:12" */
    if (!countup && secs == 0u)
    {
      big_color = RED;
      snprintf(text, text_len, "%s", cfg.cd_end[0] ? cfg.cd_end : label);
    }
    else if (days > 0u && Settings_GetDays(countup))   /* "49D LABEL" (cd days on) */
    {
      snprintf(text, text_len, label[0] ? "%luD %s" : "%lu DAYS", (unsigned long)days, label);
    }
    else
    {
      snprintf(text, text_len, "%s", label);
    }
    if (countup && cfg.cu_stopped_at != 0u && text[0] == '\0')
    {
      snprintf(text, text_len, "STOPPED");
    }
    uint32_t now;
    if (countup && cfg.cu_stopped_at == 0u && P_Now(&now) == 0 && cfg.cu_start > now)
    {
      ClockTime s;                       /* start still ahead: say when */
      ClockCalc_FromSeconds(cfg.cu_start, &s);
      if (cfg.cu_start - now < 86400u)
        snprintf(text, text_len, "STARTS %02u:%02u", s.hour, s.minute);
      else
        snprintf(text, text_len, "STARTS %04u-%02u-%02u", s.year, s.month, s.day);
    }
    if (!countup)
    {
      int running = secs > 0u;
      if (was_running == 1 && !running)
      {
        LOG_I("Countdown finished");
      }
      was_running = (int8_t)running;
    }
  }
  at_zero = (uint8_t)(!countup && big_color == RED);

  /* after zero: scene, then hold or the end screen */
  View want = VIEW_NORMAL;
  uint32_t now;
  if (at_zero && P_Now(&now) == 0)
  {
    uint32_t since = now - cfg.cd_target;
    ClockCalc_FormatBig(since, since_big, sizeof since_big);
    snprintf(end_text, sizeof end_text, "%s", cfg.cd_end[0] ? cfg.cd_end : cfg.cd_label);
    if (dismissed_target == cfg.cd_target) want = VIEW_NORMAL;   /* cleared by the user */
    else if (EndPlan_Phase(&plan, since) == END_PHASE_SCENE) want = VIEW_SCENE;
    else if (plan.then == END_THEN_SCREEN) want = VIEW_END;
  }
  if (want != view)
  {
    view = want;
    calculateClearImage();
    MsgStore_SetPlaylist(0);              /* scene: no messages; the screens set their own */
    if (view == VIEW_SCENE)    Scene_Start((SceneId)plan.scene, end_text);
    else if (view == VIEW_END) Bands_Enter(SCREEN_END);
    else                       Bands_Enter(countup ? SCREEN_COUNTUP : SCREEN_COUNTDOWN);
    if (at_zero && dismissed_target == cfg.cd_target)
    {
      MsgStore_SetPlaylist(0);            /* cleared: no messages either */
    }
  }
}

static void P_Redraw(void)
{
  view = VIEW_NONE;                     /* P_Update draws the right view */
  P_Update();
}

static void P_Draw(void)
{
  P_Update();
}

static void P_Enter(void)
{
  poll_count = 0;
  was_running = -1;
  P_Redraw();
}

int Timer_DismissEnd(void)
{
  uint32_t secs;
  if (Timer_Remaining(&secs) != 0 || secs != 0u) return -1;
  dismissed_target = cfg.cd_target;
  view = VIEW_NONE;                       /* redraw as the plain end view */
  if (!countup && Mode_Current() == &mode_countdown) P_Update();
  return 0;
}

static void P_EnterCountdown(void) { countup = 0; P_Enter(); }
static void P_EnterCountup(void)   { countup = 1; P_Enter(); }

static void P_Exit(void)
{
  editing = ED_NONE;
  guided = 0;
  TimeEdit_Stop();
  TextEdit_Stop();
  MsgStore_SetPlaylist(MSG_PLAYLIST_ALL);
}

/* Keyboard editing. Enter = guided setup: time, then label, then (countdown)
 * end text; T / L / E edit one of them. */

static void P_StartTime(void)
{
  ClockTime t;
  uint32_t now;
  uint32_t initial = countup ? cfg.cu_start : cfg.cd_target;
  if (initial == 0u)
  {
    initial = (P_Now(&now) == 0) ? now : ClockCalc_ToSeconds(&(ClockTime){ 2026, 1, 1, 0, 0, 0, 4 });
  }
  ClockCalc_FromSeconds(initial, &t);
  editing = ED_TIME;
  TimeEdit_Start(countup ? "COUNT UP FROM" : "COUNTDOWN TO", &t);
}

static void P_StartText(EditStep step)
{
  editing = step;
  if (step == ED_END)
    TextEdit_Start("END TEXT", cfg.cd_end, TIMER_TEXT_MAX);
  else
    TextEdit_Start(countup ? "COUNT-UP LABEL" : "COUNTDOWN LABEL", countup ? cfg.cu_label : cfg.cd_label,
                   TIMER_TEXT_MAX);
}

static void P_EditDone(void)
{
  editing = ED_NONE;
  guided = 0;
  TimeEdit_Stop();
  TextEdit_Stop();
  P_Redraw();
}

/* next step of the guided setup, or done */
static void P_EditNext(void)
{
  TimeEdit_Stop();
  TextEdit_Stop();
  if (guided && editing == ED_TIME)              P_StartText(ED_LABEL);
  else if (guided && editing == ED_LABEL && !countup) P_StartText(ED_END);
  else                                           P_EditDone();
}

static void P_Tick(void)
{
  if (TimeEdit_Active())
  {
    TimeEdit_Tick();
    return;
  }
  if (TextEdit_Active())
  {
    TextEdit_Tick();
    return;
  }
  if (++poll_count >= POLL_MS)
  {
    poll_count = 0;
    P_Update();
  }
  if (view == VIEW_SCENE)    Scene_Tick();
  else if (view == VIEW_END) Bands_Tick(since_big, WHITE, end_text, 1);
  else                       Bands_Tick(big, big_color, own_text, !at_zero);
}

static void P_Key(uint8_t key)
{
  if (editing == ED_TIME && TimeEdit_Active())
  {
    ClockTime t;
    TimeEntryState st = TimeEdit_Key(key, &t);
    if (st == TE_DONE)
    {
      char text[24];
      Clock_Format(&t, text, sizeof text);
      if (countup)
      {
        Timer_StartCountup(ClockCalc_ToSeconds(&t));
        LOG_I("Count-up from %s", text);
      }
      else
      {
        Timer_SetCountdown(ClockCalc_ToSeconds(&t));
        LOG_I("Countdown to %s", text);
      }
      P_EditNext();
    }
    else if (st == TE_CANCEL)
    {
      P_EditDone();                              /* ESC on the time: stop the setup */
    }
    return;
  }
  if ((editing == ED_LABEL || editing == ED_END) && TextEdit_Active())
  {
    TimeEntryState st = TextEdit_Key(key);
    if (st == TE_DONE)
    {
      const char *text = TextEdit_Text();
      if (editing == ED_END)  { Timer_SetCountdownEnd(text);   LOG_I("Countdown end text: %s", text); }
      else if (countup)       { Timer_SetCountupLabel(text);   LOG_I("Count-up label: %s", text); }
      else                    { Timer_SetCountdownLabel(text); LOG_I("Countdown label: %s", text); }
      P_EditNext();
    }
    else if (st == TE_CANCEL)
    {
      P_EditNext();                              /* ESC on a text: keep it, go on */
    }
    return;
  }

  uint32_t now;
  if (key == KB_ENTER)
  {
    guided = 1;
    P_StartTime();
  }
  else if (key == 't' || key == 'T')
  {
    guided = 0;
    P_StartTime();
  }
  else if (key == 'l' || key == 'L')
  {
    guided = 0;
    P_StartText(ED_LABEL);
  }
  else if (!countup && (key == 'e' || key == 'E'))
  {
    guided = 0;
    P_StartText(ED_END);
  }
  else if (countup && (key == 's' || key == 'S'))
  {
    if (cfg.cu_stopped_at != 0u) Timer_ResumeCountup(); else Timer_StopCountup();
    P_Draw();
  }
  else if (!countup && (key == 'c' || key == 'C' || key == KB_DEL))
  {
    if (Timer_DismissEnd() == 0) LOG_I("Countdown end show cleared from the keyboard");
  }
  else if (countup && (key == 'r' || key == 'R') && P_Now(&now) == 0)
  {
    Timer_StartCountup(now);
    LOG_I("Count-up restarted");
    P_Draw();
  }
}

const AppMode mode_countdown = {
  "countdown", "Countdown", MODE_KIND_DISPLAY, P_EnterCountdown, P_Exit, P_Tick, P_Key
};

const AppMode mode_countup = {
  "countup", "Count-up", MODE_KIND_DISPLAY, P_EnterCountup, P_Exit, P_Tick, P_Key
};

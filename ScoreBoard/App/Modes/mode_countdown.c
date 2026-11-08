/*
 * mode_countdown.c - countdown and count-up modes. See countdown.h.
 *
 * Keyboard (both modes): T or Enter opens the date/time editor (countdown:
 * the end; count-up: the start). Count-up: S stops / resumes, R restarts now.
 */
#include "countdown.h"
#include "mode.h"
#include "mode_ui.h"
#include "ui_bands.h"
#include "clock.h"
#include "clock_calc.h"
#include "ee_record.h"
#include "keys.h"
#include "log.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

#define POLL_MS        100u
#define SAVE_DELAY_MS  1000u

/* ------------------------------------------------------------ settings */

static TimerConfig cfg;
static uint8_t save_pending;
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
}

void Timer_Poll(void)
{
  if (save_pending && (int32_t)(osKernelGetTickCount() - save_at_ms) >= 0)
  {
    save_pending = 0;
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
    uint32_t days = ClockCalc_FormatDuration(secs, big, sizeof big);
    if (!countup && secs == 0u)
    {
      big_color = RED;
      snprintf(text, text_len, "%s", cfg.cd_end[0] ? cfg.cd_end : label);
    }
    else if (days > 0u)
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
}

static void P_Redraw(void)
{
  calculateClearImage();
  P_Update();
  Bands_Enter(countup ? SCREEN_COUNTUP : SCREEN_COUNTDOWN);
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

static void P_EnterCountdown(void) { countup = 0; P_Enter(); }
static void P_EnterCountup(void)   { countup = 1; P_Enter(); }

static void P_Exit(void)
{
  TimeEdit_Stop();
}

static void P_Tick(void)
{
  if (TimeEdit_Active())
  {
    TimeEdit_Tick();
    return;
  }
  if (++poll_count >= POLL_MS)
  {
    poll_count = 0;
    P_Update();
  }
  Bands_Tick(big, big_color, own_text, !at_zero);
}

static void P_Key(uint8_t key)
{
  if (TimeEdit_Active())
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
    }
    if (st != TE_EDITING)
    {
      TimeEdit_Stop();
      P_Redraw();
    }
    return;
  }

  uint32_t now;
  if (key == 't' || key == 'T' || key == KB_ENTER)
  {
    ClockTime t;
    uint32_t initial = countup ? cfg.cu_start : cfg.cd_target;
    if (initial == 0u)
    {
      initial = (P_Now(&now) == 0) ? now : ClockCalc_ToSeconds(&(ClockTime){ 2026, 1, 1, 0, 0, 0, 4 });
    }
    ClockCalc_FromSeconds(initial, &t);
    TimeEdit_Start(countup ? "COUNT UP FROM" : "COUNTDOWN TO", &t);
  }
  else if (countup && (key == 's' || key == 'S'))
  {
    if (cfg.cu_stopped_at != 0u) Timer_ResumeCountup(); else Timer_StopCountup();
    P_Draw();
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

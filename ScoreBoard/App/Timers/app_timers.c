/*
 * app_timers.c - software jobs replacing the original timer interrupts.
 *
 * Semantics kept from the StdPeriph timers the game logic was written for:
 *  - INIT_TIMER_CONFIG() starts a job and runs it once right away (SPL's
 *    TIM_TimeBaseInit generated an update event), then every period.
 *  - STOP_TIMER() stops further runs, but a first run that is still pending is
 *    executed once. The LED matrix refresh relies on this: the keyboard job
 *    (TIM7) restarts and immediately stops the refresh job (TIM5) every 4 ms.
 * All jobs run in the App task, one after the other, so they never interrupt
 * each other (as the equal-priority interrupts did before).
 */
#include "app_timers.h"
#include "TIM_CTRL.h"
#include "cmsis_os.h"

typedef struct {
  TIM_TypeDef *tim;          /* identifies the job (old timer instance) */
  void (*run)(void);         /* the original interrupt body */
  uint8_t  enabled;
  uint8_t  pending;          /* first run after a start */
  uint32_t period_ms;
  uint32_t next_ms;          /* tick of the next periodic run */
} AppJob;

/* Same order as the old NVIC order for simultaneous interrupts (by IRQ number). */
static AppJob jobs[] = {
  { TIM2, APP_TIM2_Tick, 0, 0, 1000u, 0 },
  { TIM3, APP_TIM3_Tick, 0, 0,   30u, 0 },
  { TIM4, APP_TIM4_Tick, 0, 0,  100u, 0 },
  { TIM5, APP_TIM5_Tick, 0, 0,    1u, 0 },
  { TIM7, APP_TIM7_Tick, 0, 0,    1u, 0 },
};
#define JOB_COUNT (sizeof(jobs) / sizeof(jobs[0]))

static AppJob *P_Job(TIM_TypeDef *tim)
{
  for (uint32_t i = 0; i < JOB_COUNT; i++)
  {
    if (jobs[i].tim == tim) return &jobs[i];
  }
  return NULL;
}

void INIT_TIMER_CONFIG(uint32_t TIMER_PERIPH, IRQn_Type TIMER_IRQ, TIM_TypeDef *TIM_TYPE, uint32_t TIME)
{
  (void)TIMER_PERIPH;
  (void)TIMER_IRQ;
  AppJob *job = P_Job(TIM_TYPE);
  if (job == NULL || TIME == 0u)
  {
    return;
  }
  uint32_t period = (TIME + 5u) / 10u;   /* 0.1 ms units -> ms */
  job->period_ms = (period == 0u) ? 1u : period;
  job->next_ms = osKernelGetTickCount() + job->period_ms;
  job->pending = 1;
  job->enabled = 1;
}

void STOP_TIMER(TIM_TypeDef *TIM_TYPE)
{
  AppJob *job = P_Job(TIM_TYPE);
  if (job != NULL)
  {
    job->enabled = 0;   /* a pending first run still happens once */
  }
}

void AppTimers_Run(void)
{
  for (uint32_t i = 0; i < JOB_COUNT; i++)
  {
    AppJob *job = &jobs[i];
    uint32_t now = osKernelGetTickCount();

    if (job->pending)
    {
      job->pending = 0;
      job->run();
    }
    else if (job->enabled && (int32_t)(now - job->next_ms) >= 0)
    {
      job->next_ms += job->period_ms;
      if ((int32_t)(now - job->next_ms) >= 0)
      {
        job->next_ms = now + job->period_ms;   /* fell behind: don't run a burst */
      }
      job->run();
    }
  }
}

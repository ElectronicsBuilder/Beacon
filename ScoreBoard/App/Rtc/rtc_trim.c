/*
 * rtc_trim.c - LSI measurement against the HSE and RTC trimming. See rtc_trim.h.
 */
#include "rtc_trim.h"
#include "app_hal.h"
#include "log.h"
#include "cmsis_os.h"

#define TRIM_CAPTURES     64u                    /* one capture every 8 LSI periods */
#define TRIM_LSI_PERIODS  (TRIM_CAPTURES * 8u)   /* 512 periods = about 16 ms */
#define TRIM_TIMEOUT_MS   5u                     /* per capture (8 periods = 0.25 ms) */
#define TRIM_INTERVAL_MS  (10u * 60u * 1000u)
#define TRIM_TRIES        3

static RtcTrim trim;
static uint8_t trimmed;
static uint32_t next_ms;

/* TIM5 runs on the APB1 timer clock: 2 x PCLK1 when APB1 is divided */
static uint32_t P_TimerClock(void)
{
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  return ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1) ? pclk1 : 2u * pclk1;
}

/* Last measurement, for the log and for reading with the debugger */
typedef enum { M_OK, M_TIMEOUT, M_OVERCAPTURE, M_RANGE } MeasureResult;
static const char *const result_text[] = { "ok", "no LSI capture (timeout)", "capture missed", "frequency out of range" };
volatile struct {
  uint32_t result;       /* MeasureResult */
  uint32_t captures;     /* captures taken before the result */
  uint32_t counts;       /* timer cycles over TRIM_LSI_PERIODS */
  uint32_t timer_hz;
} rtc_trim_diag;

/* Timer cycles during TRIM_LSI_PERIODS LSI periods into *counts */
static MeasureResult P_Measure(uint32_t *counts)
{
  uint32_t first = 0, last = 0, n;
  MeasureResult r = M_OK;

  __HAL_RCC_TIM5_CLK_ENABLE();
  TIM5->CR1 = 0;
  TIM5->PSC = 0;
  TIM5->ARR = 0xFFFFFFFFu;
  TIM5->OR = TIM_OR_TI4_RMP_0;                       /* TI4 = LSI */
  TIM5->CCER = 0;
  TIM5->CCMR2 = TIM_CCMR2_CC4S_0 | TIM_CCMR2_IC4PSC; /* capture TI4, every 8th edge */
  TIM5->CCER = TIM_CCER_CC4E;
  TIM5->EGR = TIM_EGR_UG;
  TIM5->SR = 0;
  TIM5->CR1 = TIM_CR1_CEN;

  osKernelLock();                                    /* no task switch for ~16 ms */
  for (n = 0; n <= TRIM_CAPTURES; n++)
  {
    uint32_t t0 = HAL_GetTick();
    while ((TIM5->SR & TIM_SR_CC4IF) == 0u)
    {
      if (HAL_GetTick() - t0 > TRIM_TIMEOUT_MS) { r = M_TIMEOUT; break; }
    }
    if (r != M_OK) break;
    if (TIM5->SR & TIM_SR_CC4OF) { r = M_OVERCAPTURE; break; }
    uint32_t c = TIM5->CCR4;                         /* reading clears CC4IF */
    if (n == 0u) first = c;
    last = c;
  }
  osKernelUnlock();

  TIM5->CR1 = 0;
  TIM5->CCER = 0;
  __HAL_RCC_TIM5_CLK_DISABLE();
  *counts = last - first;
  rtc_trim_diag.result = r;
  rtc_trim_diag.captures = n;
  rtc_trim_diag.counts = *counts;
  return r;
}

/* Write the RTC prescaler in init mode. HAL_RTC_Init() cannot be used: it skips
 * the prescaler once the calendar is set. The date and time are kept; only the
 * sub-second counter restarts. */
static int P_SetPrescaler(uint16_t a, uint16_t s)
{
  int rc = -1;
  RTC->WPR = 0xCAu;                                  /* unlock */
  RTC->WPR = 0x53u;
  RTC->ISR |= RTC_ISR_INIT;
  uint32_t t0 = HAL_GetTick();
  while ((RTC->ISR & RTC_ISR_INITF) == 0u)
  {
    if (HAL_GetTick() - t0 > 10u) goto done;
  }
  RTC->PRER = s;                                     /* two writes, as required */
  RTC->PRER = s | ((uint32_t)a << RTC_PRER_PREDIV_A_Pos);
  RTC->ISR &= ~RTC_ISR_INIT;
  RTC->ISR &= ~RTC_ISR_RSF;                          /* wait for the shadow registers */
  t0 = HAL_GetTick();
  while ((RTC->ISR & RTC_ISR_RSF) == 0u)
  {
    if (HAL_GetTick() - t0 > 10u) goto done;
  }
  hrtc.Init.AsynchPrediv = a;
  hrtc.Init.SynchPrediv = s;
  rc = 0;
done:
  RTC->ISR &= ~RTC_ISR_INIT;
  RTC->WPR = 0xFFu;                                  /* lock */
  return rc;
}

static void P_Trim(void)
{
  RtcTrim t;
  MeasureResult r = M_TIMEOUT;
  uint32_t timer_hz = P_TimerClock();
  rtc_trim_diag.timer_hz = timer_hz;
  for (int i = 0; i < TRIM_TRIES && r != M_OK; i++)
  {
    uint32_t counts;
    r = P_Measure(&counts);
    if (r == M_OK &&
        RtcTrim_Calc(timer_hz, TRIM_LSI_PERIODS, counts, trimmed ? trim.prediv_s : 0u, &t) != 0)
    {
      r = M_RANGE;
      rtc_trim_diag.result = r;
    }
  }
  if (r != M_OK)
  {
    LOG_W("RTC trim failed: %s (captures %lu, cycles %lu, timer %lu Hz)", result_text[r],
          (unsigned long)rtc_trim_diag.captures, (unsigned long)rtc_trim_diag.counts,
          (unsigned long)timer_hz);
    return;
  }

  if (!trimmed || t.prediv_s != trim.prediv_s || t.prediv_a != trim.prediv_a)
  {
    if (P_SetPrescaler(t.prediv_a, t.prediv_s) != 0)
    {
      LOG_E("RTC trim: prescaler update failed");
      return;
    }
  }
  if (!trimmed || t.calm != trim.calm || t.calp != trim.calp)
  {
    if (HAL_RTCEx_SetSmoothCalib(&hrtc, RTC_SMOOTHCALIB_PERIOD_32SEC,
                                 t.calp ? RTC_SMOOTHCALIB_PLUSPULSES_SET : RTC_SMOOTHCALIB_PLUSPULSES_RESET,
                                 t.calm) != HAL_OK)
    {
      LOG_E("RTC trim: calibration update failed");
      return;
    }
  }
  if (!trimmed)
  {
    LOG_I("RTC trimmed: LSI %lu.%03lu Hz", (unsigned long)(t.lsi_mhz / 1000u), (unsigned long)(t.lsi_mhz % 1000u));
  }
  trim = t;
  trimmed = 1;
}

void RtcTrim_Init(void)
{
  P_Trim();
  next_ms = osKernelGetTickCount() + TRIM_INTERVAL_MS;
}

void RtcTrim_Poll(void)
{
  if ((int32_t)(osKernelGetTickCount() - next_ms) >= 0)
  {
    next_ms += TRIM_INTERVAL_MS;
    P_Trim();
  }
}

int RtcTrim_Get(RtcTrim *t)
{
  if (!trimmed) return -1;
  *t = trim;
  return 0;
}

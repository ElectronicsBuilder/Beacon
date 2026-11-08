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

/* Timer cycles during TRIM_LSI_PERIODS LSI periods; 0 = failed */
static uint32_t P_Measure(void)
{
  uint32_t first = 0, last = 0, ok = 0;

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

  for (uint32_t n = 0; n <= TRIM_CAPTURES; n++)
  {
    uint32_t t0 = HAL_GetTick();
    while ((TIM5->SR & TIM_SR_CC4IF) == 0u)
    {
      if (HAL_GetTick() - t0 > TRIM_TIMEOUT_MS) goto done;
    }
    if (TIM5->SR & TIM_SR_CC4OF) goto done;          /* a capture was missed (task preempted) */
    uint32_t c = TIM5->CCR4;                         /* reading clears CC4IF */
    if (n == 0u) first = c;
    last = c;
  }
  ok = 1;

done:
  TIM5->CR1 = 0;
  TIM5->CCER = 0;
  __HAL_RCC_TIM5_CLK_DISABLE();
  return ok ? (last - first) : 0u;
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
  int ok = -1;
  for (int i = 0; i < TRIM_TRIES && ok != 0; i++)
  {
    uint32_t counts = P_Measure();
    if (counts != 0u)
    {
      ok = RtcTrim_Calc(P_TimerClock(), TRIM_LSI_PERIODS, counts, trimmed ? trim.prediv_s : 0u, &t);
    }
  }
  if (ok != 0)
  {
    LOG_W("RTC trim: LSI measurement failed");
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

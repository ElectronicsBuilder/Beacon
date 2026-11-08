/*
 * rtc_trim_calc.c - RTC prescaler and smooth calibration from a measured LSI.
 * Pure integer maths, unit tested on the PC (tests/host).
 */
#include "rtc_trim_calc.h"

#define LSI_MIN_HZ  15000u
#define LSI_MAX_HZ  50000u
#define CAL_SCALE   (1LL << 20)

/* X = CALM - 512 * CALP that makes f / (4 * (S + 1)) exactly 1 Hz */
static int64_t P_CalX(uint64_t cycles_x_hz, uint64_t counts, uint32_t s_plus_1)
{
  uint64_t den = (uint64_t)(RTC_TRIM_PREDIV_A + 1u) * s_plus_1 * counts;
  int64_t num = (int64_t)cycles_x_hz - (int64_t)den;      /* f - 4(S+1), scaled by counts */
  int64_t scaled = num * CAL_SCALE;
  int64_t half = (int64_t)(den / 2u);
  return (scaled >= 0) ? (scaled + half) / (int64_t)den : -((-scaled + half) / (int64_t)den);
}

static int P_Fill(int64_t x, uint32_t s_plus_1, RtcTrim *out)
{
  if (x < -512 || x > 511)
  {
    return -1;
  }
  out->prediv_a = (uint16_t)RTC_TRIM_PREDIV_A;
  out->prediv_s = (uint16_t)(s_plus_1 - 1u);
  out->calp = (x < 0) ? 1u : 0u;
  out->calm = (uint16_t)(x < 0 ? x + 512 : x);
  return 0;
}

int RtcTrim_Calc(uint32_t timer_hz, uint32_t lsi_periods, uint32_t timer_counts,
                 uint16_t keep_prediv_s, RtcTrim *out)
{
  if (timer_counts == 0u || lsi_periods == 0u)
  {
    return -1;
  }
  uint64_t cycles = (uint64_t)timer_hz * lsi_periods;    /* f_lsi * counts */
  uint64_t f_mhz = (cycles * 1000u) / timer_counts;
  if (f_mhz < (uint64_t)LSI_MIN_HZ * 1000u || f_mhz > (uint64_t)LSI_MAX_HZ * 1000u)
  {
    return -1;
  }
  out->lsi_mhz = (uint32_t)f_mhz;

  /* keep the prescaler in use if the calibration still reaches 1 Hz */
  if (keep_prediv_s != 0u &&
      P_Fill(P_CalX(cycles, timer_counts, keep_prediv_s + 1u), keep_prediv_s + 1u, out) == 0)
  {
    return 0;
  }
  /* otherwise the nearest prescaler: remaining error within +-63 ppm */
  uint32_t div_a = RTC_TRIM_PREDIV_A + 1u;
  uint32_t s_plus_1 = (uint32_t)((cycles + (uint64_t)div_a * timer_counts / 2u) /
                                 ((uint64_t)div_a * timer_counts));
  return P_Fill(P_CalX(cycles, timer_counts, s_plus_1), s_plus_1, out);
}

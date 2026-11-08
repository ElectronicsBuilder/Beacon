/*
 * rtc_trim_calc.h - RTC prescaler and smooth calibration from a measured LSI.
 *
 * The RTC runs from the LSI oscillator (no 32.768 kHz crystal fitted), which
 * is only specified as 17..47 kHz. Its real frequency is measured against the
 * HSE crystal (TIM5 input capture, Rtc/rtc_trim.c); this computes the RTC
 * settings that give 1 Hz:
 *   1 Hz = f_lsi * 2^20 / (2^20 + CALM - 512 * CALP) / ((A + 1) * (S + 1))
 * with A = 3 (CALP needs A >= 3), S chosen so the remaining error is within
 * the smooth calibration range (about +-488 ppm, 0.95 ppm steps).
 */
#ifndef APP_RTC_TRIM_CALC_H
#define APP_RTC_TRIM_CALC_H

#include <stdint.h>

#define RTC_TRIM_PREDIV_A   3u          /* asynchronous prescaler: divide by 4 */

typedef struct {
  uint32_t lsi_mhz;      /* measured LSI frequency in millihertz */
  uint16_t prediv_a;     /* RTC_PRER PREDIV_A (divide by value + 1) */
  uint16_t prediv_s;     /* RTC_PRER PREDIV_S (divide by value + 1) */
  uint8_t  calp;         /* RTC_CALR CALP: add 512 pulses per 2^20 */
  uint16_t calm;         /* RTC_CALR CALM: remove 0..511 pulses per 2^20 */
} RtcTrim;

/*
 * timer_counts timer clock cycles (timer_hz) were measured over lsi_periods
 * LSI periods. keep_prediv_s: the prescaler in use (0 = none); it is kept when
 * the calibration can still reach 1 Hz with it, because changing the prescaler
 * restarts the RTC's sub-second counter. Returns 0, or -1 if the measurement
 * is outside the LSI range.
 */
int RtcTrim_Calc(uint32_t timer_hz, uint32_t lsi_periods, uint32_t timer_counts,
                 uint16_t keep_prediv_s, RtcTrim *out);

#endif

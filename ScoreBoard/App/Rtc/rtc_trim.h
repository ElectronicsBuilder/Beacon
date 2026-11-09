/*
 * rtc_trim.h - keeps the LSI-clocked RTC accurate by measuring the LSI against
 * the HSE crystal (TIM5 channel 4 input capture, LSI remapped internally).
 *
 * Measured at boot and every 10 minutes (the LSI drifts with temperature);
 * the RTC prescaler and smooth calibration are set from the result, so the
 * clock is about as accurate as the 8 MHz crystal (tens of ppm, a few s/day).
 *
 * The prescaler is only written when it has to change (that stops the RTC
 * briefly and restarts its sub-second counter); after a reset the value
 * already in the RTC is kept, so a reboot does not cost time. Note: the LSI
 * only runs on main power. On VBAT alone the RTC keeps the date and time but
 * does not count (that needs a 32.768 kHz LSE crystal).
 *
 * TIM5 is used here directly (registers), it is not enabled in the .ioc.
 * Do not assign TIM5 to anything else in CubeMX.
 */
#ifndef APP_RTC_TRIM_H
#define APP_RTC_TRIM_H

#include <stdint.h>
#include "rtc_trim_calc.h"

void RtcTrim_Init(void);                     /* measure and trim (App_Init) */
void RtcTrim_Poll(void);                     /* App task: re-trim every 10 min */
int  RtcTrim_Get(RtcTrim *t);                /* 0 = trimmed, *t = settings in use */
int  RtcTrim_Enabled(void);
void RtcTrim_SetEnabled(int on);             /* off = CubeMX default prescaler; saved */

#endif

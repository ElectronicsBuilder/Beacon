//--------------------------------------------------------------
// File     : rtc_clock.h
// Game clock based on the internal RTC (LSI clock, set up by CubeMX).
// TIM2 fires once per second (RTC_UPDATE_TIME = 10000 x 0.1 ms) and the
// handler in TIM_CTRL.c reads the time with RTC_TimeShow().
//--------------------------------------------------------------
#ifndef APP_RTC_CLOCK_H
#define APP_RTC_CLOCK_H

#include "app_hal.h"
#include "app_timers.h"


#define RTC_TIMER_PERIPH   RCC_APB1Periph_TIM2
#define RTC_TIMER_IRQ      TIM2_IRQn
#define RTC_TIM_TYPE       TIM2
#define RTC_UPDATE_TIME    10000

#define FIRST_DATA         0x32F2   // marker written to backup register 0

void RTC_Configuration(void);
void RTC_RESET_TIME(void);
void RTC_TimeShow(void);
void RTC_SetMinutes(uint8_t minutes_bin);
void RTC_SetHours(uint8_t hours_bcd);

uint8_t ByteToBcd(uint8_t Value);
uint8_t Bcd2ToByte(uint8_t Value);

#endif // APP_RTC_CLOCK_H

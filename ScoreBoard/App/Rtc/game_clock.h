//--------------------------------------------------------------
// File     : game_clock.h
// Game timer shown as "TM mm:ss" on the matrix (was the RTC in 2016).
// It counts the time since the game started; the hardware RTC is now the
// wall clock (Services/clock.c). TIM2 job refreshes the display once a second.
//--------------------------------------------------------------
#ifndef APP_GAME_CLOCK_H
#define APP_GAME_CLOCK_H

#include "app_hal.h"
#include "app_timers.h"

#define RTC_TIMER_PERIPH   RCC_APB1Periph_TIM2
#define RTC_TIMER_IRQ      TIM2_IRQn
#define RTC_TIM_TYPE       TIM2
#define RTC_UPDATE_TIME    10000

void RTC_Configuration(void);           // start-up: reset the game timer
void RTC_RESET_TIME(void);              // game start: timer to 00:00:00
void RTC_TimeShow(void);                // update SEC_H / MIN_H / HOUR_H (BCD)
void RTC_SetMinutes(uint8_t minutes_bin);
void RTC_SetHours(uint8_t hours_bcd);

uint8_t ByteToBcd(uint8_t Value);
uint8_t Bcd2ToByte(uint8_t Value);

#endif // APP_GAME_CLOCK_H

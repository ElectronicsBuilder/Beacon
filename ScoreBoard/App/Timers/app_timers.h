/*
 * app_timers.h - periodic interrupt timers of the original firmware (was TIMERS.h).
 *
 * TIM2/3/4/5/7 are set up by CubeMX with an 8400 prescaler (10 kHz tick from the
 * 84 MHz APB1 timer clock, same as the old code), so TIME is still in 0.1 ms units.
 *   TIM2: RTC/clock display refresh   TIM3: on-the-fly messages
 *   TIM4: display update              TIM5: data update   TIM7: USB keyboard poll
 */
#ifndef APP_TIMERS_H
#define APP_TIMERS_H

#include "app_hal.h"

void INIT_TIMER_CONFIG(uint32_t TIMER_PERIPH, IRQn_Type TIMER_IRQ, TIM_TypeDef *TIM_TYPE, uint32_t TIME);
void STOP_TIMER(TIM_TypeDef *TIM_TYPE);

#endif

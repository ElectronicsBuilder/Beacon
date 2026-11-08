/*
 * app_timers.h - periodic jobs of the original firmware (was TIMERS.h).
 *
 * The 2016 firmware ran its game logic from five hardware timer interrupts.
 * They are now software jobs executed by the App task (App_Run), in the same
 * order and at the same periods. The API is unchanged; TIME is in the old
 * 0.1 ms units, the TIMx argument selects the job:
 *   TIM2: clock display (1 s)          TIM3: scrolling messages (30 ms)
 *   TIM4: display update (100 ms)      TIM5: LED matrix data refresh
 *   TIM7: keyboard poll (1 ms)
 */
#ifndef APP_TIMERS_H
#define APP_TIMERS_H

#include "app_hal.h"

void INIT_TIMER_CONFIG(uint32_t TIMER_PERIPH, IRQn_Type TIMER_IRQ, TIM_TypeDef *TIM_TYPE, uint32_t TIME);
void STOP_TIMER(TIM_TypeDef *TIM_TYPE);

/* Called by the App task once per millisecond: runs every job that is due. */
void AppTimers_Run(void);

#endif

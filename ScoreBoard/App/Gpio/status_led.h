//--------------------------------------------------------------
// File     : status_led.h  (status LEDs PE2..PE5, HAL version)
//--------------------------------------------------------------
#ifndef __APP_LED_H
#define __APP_LED_H

#include "app_hal.h"

typedef enum
{
  LED_GREEN = 0,  // PE2
  LED_ORANGE = 1, // PE3
  LED_RED = 2,    // PE4
  LED_BLUE = 3    // PE5
} LED_NAME_t;

#define LED_ANZ 4

typedef enum {
  LED_OFF = 0,
  LED_ON
} LED_STATUS_t;

void Led_Init(void);
void Led_Off(LED_NAME_t led_name);
void Led_On(LED_NAME_t led_name);
void Led_Toggle(LED_NAME_t led_name);
void Led_Switch(LED_NAME_t led_name, LED_STATUS_t wert);

#endif // __APP_LED_H

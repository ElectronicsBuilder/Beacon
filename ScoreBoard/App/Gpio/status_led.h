//--------------------------------------------------------------
// File     : status_led.h  (status LEDs PE2..PE5)
//
// The four board LEDs, dimmed by software PWM (TIM7 interrupt, 32 kHz,
// 128 levels = 250 Hz, no visible flicker). PE2..PE4 have no timer output,
// so hardware PWM is not possible for all of them.
//
// What they show (Led_Poll, App task):
//   green   heartbeat while the App task runs: slow fade or a short blink
//           every second (setting; frozen = firmware stuck)
//   blue    Bluetooth: off = no module, dim = waiting, bright = connected,
//           flicker = data
//   orange  attention: blinks while the clock needs syncing ("SYNC TIME");
//           flashes on keyboard keys and console commands
//   red     errors, as blink codes (n blinks, pause): 1 EEPROM missing,
//           2 Bluetooth module not answering, 3 RTC trim failed;
//           solid = crashed (HardFault)
// All four use one brightness (console "led bright", saved).
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

#define LED_BRIGHTNESS_DEFAULT 15     /* percent */

typedef enum { LED_HB_FADE = 0, LED_HB_BLINK = 1, LED_HB_COUNT } LedHeartbeat;

void Led_Init(void);                  /* pins off, start the PWM timer */
void Led_Poll(void);                  /* App task, every 1 ms: the patterns above */
void Led_Flash(LED_NAME_t led, uint16_t ms);   /* activity flash */
void Led_SetBrightness(uint8_t percent);       /* 0..100, 0 = all off */
uint8_t Led_GetBrightness(void);
void Led_SetHeartbeat(uint8_t style);          /* LedHeartbeat */
uint8_t Led_GetHeartbeat(void);
const char *Led_HeartbeatName(uint8_t style);  /* "fade" / "blink" */
void Led_Test(void);                  /* each LED in turn for 1 s, then normal */
uint8_t Led_Errors(void);             /* bit 0 EEPROM, 1 Bluetooth, 2 RTC trim */
void Led_Crash(void);                 /* HardFault: red on, others off (no PWM) */

/* direct control, kept for old code: full brightness / off */
void Led_Off(LED_NAME_t led_name);
void Led_On(LED_NAME_t led_name);
void Led_Toggle(LED_NAME_t led_name);
void Led_Switch(LED_NAME_t led_name, LED_STATUS_t wert);

#endif // __APP_LED_H

//--------------------------------------------------------------
// File     : dig_out.h  (digital outputs, HAL version)
//--------------------------------------------------------------
#ifndef __APP_DIG_OUT_H
#define __APP_DIG_OUT_H

#include "app_hal.h"

typedef enum
{
  RADIO_EN = 0,      // PA7  HM-TRP enable
  RADIO_CONFIG = 1,  // PA1  HM-TRP config
  EE_WP = 3,         // PB7  EEPROM write protect
  USB_ON = 4,        // PC0  USB host power switch enable (active low)
  LED_POWER = 5      // PB0  LED supply regulator enable (REG_EN)
} DOUT_NAME_t;

void DigOut_Init(void);
void DigOut_Lo(DOUT_NAME_t dig_pin);
void DigOut_Hi(DOUT_NAME_t dig_pin);
void DigOut_Toggle(DOUT_NAME_t dig_pin);
void DigOut_Pin(DOUT_NAME_t dig_pin, BitAction wert);

#endif // __APP_DIG_OUT_H

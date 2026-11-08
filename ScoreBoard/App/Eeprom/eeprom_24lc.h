//--------------------------------------------------------------
// File     : eeprom_24lc.h
// Byte access to the 24LCxx I2C EEPROM (16-bit memory address) on I2C1.
// Used by GAME.c for the player records and the all-time score table.
//--------------------------------------------------------------
#ifndef APP_EEPROM_24LC_H
#define APP_EEPROM_24LC_H

#include "app_hal.h"
#include "i2c1_bus.h"

// 8-bit slave addresses selected by the A2..A0 pins (board: all low -> 0xA0)
typedef enum {
  EE24_ADDR_0 = 0xA0, // A2=0, A1=0, A0=0
  EE24_ADDR_1 = 0xA2, // A2=0, A1=0, A0=1
  EE24_ADDR_2 = 0xA4, // A2=0, A1=1, A0=0
  EE24_ADDR_3 = 0xA6, // A2=0, A1=1, A0=1
  EE24_ADDR_4 = 0xA8, // A2=1, A1=0, A0=0
  EE24_ADDR_5 = 0xAA, // A2=1, A1=0, A0=1
  EE24_ADDR_6 = 0xAC, // A2=1, A1=1, A0=0
  EE24_ADDR_7 = 0xAE  // A2=1, A1=1, A0=1
} EE24_ADDR_t;

#define EE24_WRITE_TIME_MS  10u   // max. internal write cycle time (24LCxx: 5 ms)

ErrorStatus Eeprom24_Init(EE24_ADDR_t slave);
int16_t Eeprom24_ReadByte(EE24_ADDR_t slave, uint16_t adr);              // 0..255, <0 = error
int16_t Eeprom24_WriteByte(EE24_ADDR_t slave, uint16_t adr, uint8_t wert); // 0 = ok, <0 = error

#endif // APP_EEPROM_24LC_H

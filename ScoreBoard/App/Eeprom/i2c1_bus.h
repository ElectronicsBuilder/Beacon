//--------------------------------------------------------------
// File     : i2c1_bus.h  (I2C1 for the 24LCxx EEPROM, HAL version)
// SCL/SDA pins and the 100 kHz clock are configured by CubeMX (MX_I2C1_Init).
// Return values as before: >=0 data/OK, <0 error.
//--------------------------------------------------------------
#ifndef __STM32F4_I2C1Bus_H
#define __STM32F4_I2C1Bus_H

#include "app_hal.h"


int16_t I2C1Bus_ReadByte16(uint8_t slave_adr, uint16_t adr);
int16_t I2C1Bus_WriteByte16(uint8_t slave_adr, uint16_t adr, uint8_t wert);
// Block read / write (write: within one EEPROM page, caller waits for ready)
int16_t I2C1Bus_Read16(uint8_t slave_adr, uint16_t adr, uint8_t *buf, uint16_t len);
int16_t I2C1Bus_Write16(uint8_t slave_adr, uint16_t adr, const uint8_t *buf, uint16_t len);
// Wait until an EEPROM finishes its internal write cycle (ACK polling).
// Call from the App task (uses osDelay).
int16_t I2C1Bus_WaitReady(uint8_t slave_adr, uint32_t timeout_ms);

#endif // __STM32F4_I2C1Bus_H

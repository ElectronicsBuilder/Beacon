//--------------------------------------------------------------
// File     : i2c1_bus.h  (I2C1 for the 24LCxx EEPROM, HAL version)
// SCL/SDA pins and the 100 kHz clock are configured by CubeMX (MX_I2C1_Init).
// Return values as before: >=0 data/OK, <0 error.
//--------------------------------------------------------------
#ifndef __STM32F4_I2C1Bus_H
#define __STM32F4_I2C1Bus_H

#include "app_hal.h"

#define I2C1_MULTIBYTE_ANZ 10
uint8_t I2C1_DATA[I2C1_MULTIBYTE_ANZ];

void I2C1Bus_Init(void);
int16_t I2C1Bus_ReadByte(uint8_t slave_adr, uint8_t adr);
int16_t I2C1Bus_WriteByte(uint8_t slave_adr, uint8_t adr, uint8_t wert);
int16_t I2C1Bus_ReadMultiByte(uint8_t slave_adr, uint8_t adr, uint8_t cnt);
int16_t I2C1Bus_WriteMultiByte(uint8_t slave_adr, uint8_t adr, uint8_t cnt);
int16_t I2C1Bus_WriteCMD(uint8_t slave_adr, uint8_t cmd);
int16_t I2C1Bus_ReadByte16(uint8_t slave_adr, uint16_t adr);
int16_t I2C1Bus_WriteByte16(uint8_t slave_adr, uint16_t adr, uint8_t wert);
void I2C1Bus_Delay(volatile uint32_t nCount);
// Wait until an EEPROM finishes its internal write cycle (ACK polling).
// Call from the App task (uses osDelay).
int16_t I2C1Bus_WaitReady(uint8_t slave_adr, uint32_t timeout_ms);

#endif // __STM32F4_I2C1Bus_H

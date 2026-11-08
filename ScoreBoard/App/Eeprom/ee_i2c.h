/*
 * ee_i2c.h - I2C transfer helpers used by EEPROM.c (was EEPROM/i2c.h).
 * Implemented on the CubeMX I2C1 handle. Return OK (0) / ERROR (-1).
 * Device addresses are 8-bit (0xA0). ptrlen = number of address bytes (0..2).
 */
#ifndef EE_I2C_H
#define EE_I2C_H

#include "app_hal.h"
#include <stdint.h>

void I2C_BusInit(void);
int  I2C_CheckDevice(uint8_t dev_addr);
int  I2C_ReadTransfer(uint8_t dev_addr, uint8_t *buffer, int cnt, uint32_t ptr, uint8_t ptrlen);
int  I2C_WriteTransfer(uint8_t dev_addr, uint8_t *buffer, int cnt, uint32_t ptr, uint8_t ptrlen);

#endif

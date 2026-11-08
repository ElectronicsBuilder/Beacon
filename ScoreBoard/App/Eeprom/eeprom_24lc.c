//--------------------------------------------------------------
// File     : eeprom_24lc.c
// Byte access to the 24LCxx I2C EEPROM (16-bit memory address) on I2C1.
//--------------------------------------------------------------
#include "eeprom_24lc.h"

// Check that the EEPROM answers by reading address 0.
ErrorStatus Eeprom24_Init(EE24_ADDR_t slave)
{
  return (I2C1Bus_ReadByte16(slave, 0x0000) >= 0) ? SUCCESS : ERROR;
}

int16_t Eeprom24_ReadByte(EE24_ADDR_t slave, uint16_t adr)
{
  return I2C1Bus_ReadByte16(slave, adr);
}

// Write one byte, then wait until the EEPROM has finished its write cycle
// (the original used a fixed busy-wait here).
int16_t Eeprom24_WriteByte(EE24_ADDR_t slave, uint16_t adr, uint8_t wert)
{
  int16_t ret = I2C1Bus_WriteByte16(slave, adr, wert);
  if (ret >= 0)
  {
    I2C1Bus_WaitReady(slave, EE24_WRITE_TIME_MS);
  }
  return ret;
}

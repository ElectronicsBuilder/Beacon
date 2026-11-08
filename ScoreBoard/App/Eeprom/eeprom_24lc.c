//--------------------------------------------------------------
// File     : eeprom_24lc.c
// Byte access to the 24LCxx I2C EEPROM (16-bit memory address) on I2C1.
//--------------------------------------------------------------
#include "eeprom_24lc.h"
#include <string.h>

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

int16_t Eeprom24_Read(EE24_ADDR_t slave, uint16_t adr, void *buf, uint16_t len)
{
  return I2C1Bus_Read16(slave, adr, (uint8_t *)buf, len);
}

int16_t Eeprom24_Write(EE24_ADDR_t slave, uint16_t adr, const void *buf, uint16_t len)
{
  const uint8_t *p = (const uint8_t *)buf;
  uint8_t old[EE24_PAGE_SIZE];
  while (len > 0u)
  {
    uint16_t n = (uint16_t)(EE24_PAGE_SIZE - (adr % EE24_PAGE_SIZE));   /* to the page end */
    if (n > len) n = len;
    if (I2C1Bus_Read16(slave, adr, old, n) != 0 || memcmp(old, p, n) != 0)
    {
      if (I2C1Bus_Write16(slave, adr, p, n) != 0) return -1;
      if (I2C1Bus_WaitReady(slave, EE24_WRITE_TIME_MS) != 0) return -1;
    }
    adr = (uint16_t)(adr + n);
    p += n;
    len = (uint16_t)(len - n);
  }
  return 0;
}

uint32_t Eeprom24_Size(EE24_ADDR_t slave)
{
  static uint32_t size;
  const uint16_t probe = 0x0FFFu;            /* last byte of the first 4 KB (spare) */
  if (size != 0u) return size;

  int16_t orig = Eeprom24_ReadByte(slave, probe);
  if (orig < 0) return 0;
  size = 0x10000u;                           /* no wrap found below 64 KB */
  for (uint32_t n = 0x1000u; n < 0x10000u; n <<= 1)
  {
    int16_t high = Eeprom24_ReadByte(slave, (uint16_t)(probe + n));
    if (high != orig) continue;              /* different content: not a mirror */
    uint8_t mark = (uint8_t)(orig ^ 0x5Au);
    Eeprom24_WriteByte(slave, probe, mark);
    int mirrored = Eeprom24_ReadByte(slave, (uint16_t)(probe + n)) == mark;
    Eeprom24_WriteByte(slave, probe, (uint8_t)orig);
    if (mirrored)
    {
      size = n;
      break;
    }
  }
  return size;
}

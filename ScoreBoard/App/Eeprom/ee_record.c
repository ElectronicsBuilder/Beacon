/*
 * ee_record.c - EEPROM records with length and CRC. See ee_record.h.
 */
#include "ee_record.h"
#include "eeprom_24lc.h"
#include "crc16.h"

static int P_WriteIfChanged(uint16_t addr, uint8_t value)
{
  int16_t old = Eeprom24_ReadByte(EE24_ADDR_0, addr);
  if (old == value) return 0;
  return Eeprom24_WriteByte(EE24_ADDR_0, addr, value) == 0 ? 0 : -1;
}

int EeRecord_Load(uint16_t addr, void *data, size_t len)
{
  uint8_t *p = (uint8_t *)data;
  if (len > 250u || Eeprom24_ReadByte(EE24_ADDR_0, addr) != (int16_t)len)
  {
    return -1;
  }
  for (size_t i = 0; i < len; i++)
  {
    int16_t b = Eeprom24_ReadByte(EE24_ADDR_0, (uint16_t)(addr + 1u + i));
    if (b < 0) return -1;
    p[i] = (uint8_t)b;
  }
  int16_t lo = Eeprom24_ReadByte(EE24_ADDR_0, (uint16_t)(addr + 1u + len));
  int16_t hi = Eeprom24_ReadByte(EE24_ADDR_0, (uint16_t)(addr + 2u + len));
  if (lo < 0 || hi < 0) return -1;
  return (uint16_t)(lo | (hi << 8)) == Crc16(data, len) ? 0 : -1;
}

int EeRecord_Save(uint16_t addr, const void *data, size_t len)
{
  const uint8_t *p = (const uint8_t *)data;
  uint16_t crc = Crc16(data, len);
  int rc = 0;
  if (len > 250u) return -1;
  rc |= P_WriteIfChanged(addr, (uint8_t)len);
  for (size_t i = 0; i < len; i++)
  {
    rc |= P_WriteIfChanged((uint16_t)(addr + 1u + i), p[i]);
  }
  rc |= P_WriteIfChanged((uint16_t)(addr + 1u + len), (uint8_t)crc);
  rc |= P_WriteIfChanged((uint16_t)(addr + 2u + len), (uint8_t)(crc >> 8));
  return rc;
}

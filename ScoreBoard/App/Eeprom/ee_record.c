/*
 * ee_record.c - EEPROM records with length and CRC. See ee_record.h.
 */
#include "ee_record.h"
#include "eeprom_24lc.h"
#include "crc16.h"
#include <string.h>

#define EE_RECORD_MAX 250u

int EeRecord_Load(uint16_t addr, void *data, size_t len)
{
  uint8_t buf[EE_RECORD_MAX + 3u];
  if (len > EE_RECORD_MAX) return -1;
  if (Eeprom24_Read(EE24_ADDR_0, addr, buf, (uint16_t)(len + 3u)) != 0) return -1;
  if (buf[0] != (uint8_t)len) return -1;
  uint16_t crc = (uint16_t)(buf[len + 1u] | (buf[len + 2u] << 8));
  if (crc != Crc16(&buf[1], len)) return -1;
  memcpy(data, &buf[1], len);
  return 0;
}

int EeRecord_Save(uint16_t addr, const void *data, size_t len)
{
  uint8_t buf[EE_RECORD_MAX + 3u];
  if (len > EE_RECORD_MAX) return -1;
  uint16_t crc = Crc16(data, len);
  buf[0] = (uint8_t)len;
  memcpy(&buf[1], data, len);
  buf[len + 1u] = (uint8_t)crc;
  buf[len + 2u] = (uint8_t)(crc >> 8);
  return Eeprom24_Write(EE24_ADDR_0, addr, buf, (uint16_t)(len + 3u)) == 0 ? 0 : -1;
}

int EeRecord_Erase(uint16_t addr)
{
  return Eeprom24_WriteByte(EE24_ADDR_0, addr, 0xFFu) == 0 ? 0 : -1;   /* invalid length */
}

/*
 * ee_record.h - small records in the EEPROM with a length and CRC check.
 *
 * Stored as: [length][data ... ][CRC-16 low][CRC-16 high] at a fixed address,
 * so a record needs len + 3 bytes. Only bytes that differ are written (each
 * byte write takes up to 5 ms). A record that was never written, has another
 * length (layout changed) or a bad CRC (power lost while writing) is reported
 * as missing and the caller uses its defaults.
 *
 * EEPROM map (24LCxx, 0xA0):
 *   0x0000..0x05FF  player records and all-time scores (Game/GAME.c)
 *   0x0E00..0x0EFF  countdown / count-up (Modes/mode_countdown.c)
 *   0x0F00..0x0F0F  settings (Eeprom/settings.c)
 */
#ifndef APP_EE_RECORD_H
#define APP_EE_RECORD_H

#include <stdint.h>
#include <stddef.h>

#define EE_RECORD_TIMER_ADDR  0x0E00u

int EeRecord_Load(uint16_t addr, void *data, size_t len);        /* 0 = ok */
int EeRecord_Save(uint16_t addr, const void *data, size_t len);  /* 0 = ok */

#endif

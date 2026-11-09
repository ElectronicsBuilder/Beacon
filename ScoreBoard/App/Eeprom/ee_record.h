/*
 * ee_record.h - small records in the EEPROM with a length and CRC check.
 *
 * Stored as: [length][data ... ][CRC-16 low][CRC-16 high] at a fixed address,
 * so a record needs len + 3 bytes (data up to 250 bytes). Written in EEPROM
 * pages; pages that are already equal are skipped. A record that was never written, has another
 * length (layout changed) or a bad CRC (power lost while writing) is reported
 * as missing and the caller uses its defaults.
 *
 * EEPROM map (24LCxx, 0xA0):
 *   0x0000..0x05FF  player records and all-time scores (Game/GAME.c)
 *   0x0600..0x0DFF  messages, 96-byte slots (Services/msg_store.c)
 *   0x0E00..0x0EBF  countdown / count-up (Modes/mode_countdown.c)
 *   0x0EC0..0x0EDF  message playback settings (Services/msg_store.c)
 *   0x0F00..0x0F0F  settings (Eeprom/settings.c)
 *   0x0F40..0x0F7F  screen line layouts (Modes/screen_layout.c)
 *   0x0F80..0x0F8F  countdown end plan (Modes/mode_countdown.c)
 *   0x0FFF          spare, used by the size detection (Eeprom24_Size)
 *   0x1000..end     more message slots on EEPROMs larger than 4 KB
 */
#ifndef APP_EE_RECORD_H
#define APP_EE_RECORD_H

#include <stdint.h>
#include <stddef.h>

#define EE_RECORD_TIMER_ADDR   0x0E00u
#define EE_RECORD_MSGCFG_ADDR  0x0EC0u
#define EE_RECORD_SCREEN_ADDR  0x0F40u
#define EE_RECORD_ENDPLAN_ADDR 0x0F80u

int EeRecord_Load(uint16_t addr, void *data, size_t len);        /* 0 = ok */
int EeRecord_Save(uint16_t addr, const void *data, size_t len);  /* 0 = ok */
int EeRecord_Erase(uint16_t addr);                                /* mark as missing */

#endif

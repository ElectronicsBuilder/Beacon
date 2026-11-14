/*
 * w25q.h - Winbond W25Q16JV SPI NOR flash (2 MB) on SPI4.
 *
 * Pins: SCK PE12, MISO PE13, MOSI PE14 (SPI4, mode 0, 21 MHz); chip select
 * FLASH_CE (PE15) or SPI4_CS (PE11), found at start-up by reading the
 * JEDEC ID; WP (PE9) and HOLD (PE10) held high.
 *
 * Organisation: 256-byte pages, 4 KB sectors (erase unit), 512 sectors.
 * Erase sets bytes to 0xFF; programming can only clear bits.
 */
#ifndef APP_W25Q_H
#define APP_W25Q_H

#include <stdint.h>
#include <stddef.h>

#define W25Q_PAGE     256u
#define W25Q_SECTOR   4096u

int      Flash_Init(void);                       /* 0 = chip found */
int      Flash_Present(void);
uint32_t Flash_Id(void);                         /* JEDEC: manufacturer, type, capacity */
uint32_t Flash_Size(void);                       /* bytes (from the ID), 0 = none */
const char *Flash_CsName(void);                  /* which chip select answered */

int Flash_Read(uint32_t addr, void *buf, size_t len);
int Flash_Write(uint32_t addr, const void *buf, size_t len);   /* erased area; pages split */
int Flash_EraseSector(uint32_t addr);                          /* 4 KB containing addr */

#endif

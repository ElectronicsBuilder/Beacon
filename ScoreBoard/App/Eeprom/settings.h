/*
 * settings.h - user settings kept in the EEPROM.
 *
 * Layout (outside the game's player records, which end around 0x05A0):
 *   0x0F00  marker 0xB7 (settings valid)
 *   0x0F01  LED matrix brightness in percent
 *   0x0F02  display mode (index in the mode table, 0xFF = not saved)
 */
#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdint.h>

void Settings_Load(void);                 /* at boot, after the EEPROM is ready */
void Settings_BrightnessChanged(void);    /* call after a change; saved 2 s later */
int  Settings_GetMode(void);              /* saved mode index, -1 = none */
void Settings_SetMode(uint8_t mode);      /* saved 2 s later */
void Settings_Poll(void);                 /* App task: performs the delayed save */

#endif

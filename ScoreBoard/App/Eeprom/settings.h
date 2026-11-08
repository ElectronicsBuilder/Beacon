/*
 * settings.h - user settings kept in the EEPROM (currently: matrix brightness).
 *
 * Layout (outside the game's player records, which end around 0x05A0):
 *   0x0F00  marker 0xB7 (settings valid)
 *   0x0F01  LED matrix brightness in percent
 */
#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdint.h>

void Settings_Load(void);                 /* at boot, after the EEPROM is ready */
void Settings_BrightnessChanged(void);    /* call after +/- ; saved 2 s later */
void Settings_Poll(void);                 /* App task: performs the delayed save */

#endif

/*
 * ui_menu.h - settings menu on the matrix, opened with F12 from any mode.
 *
 * Up/Down choose the setting, Left/Right change it (or type a number),
 * Enter / ESC / F12 close it; changes are saved when it closes.
 * Settings: message gap, message time, ticker on/off, play order, brightness.
 */
#ifndef APP_UI_MENU_H
#define APP_UI_MENU_H

#include <stdint.h>

void Menu_Open(void);              /* clears the screen */
int  Menu_Active(void);
void Menu_Tick(void);
int  Menu_Key(uint8_t key);        /* 1 = menu closed by this key */

#endif

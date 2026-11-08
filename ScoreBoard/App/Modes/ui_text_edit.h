/*
 * ui_text_edit.h - full-screen one-line text editor for the keyboard.
 * Title on the upper line, the text on the lower line with a blinking cursor
 * (the last 15 characters are shown while typing longer text).
 */
#ifndef APP_UI_TEXT_EDIT_H
#define APP_UI_TEXT_EDIT_H

#include "text_entry.h"

void TextEdit_Start(const char *title, const char *initial, uint8_t max);   /* clears the screen */
int  TextEdit_Active(void);
void TextEdit_Stop(void);
void TextEdit_Tick(void);
TimeEntryState TextEdit_Key(uint8_t key);      /* TE_DONE: text in TextEdit_Text(), editor stays open */
const char *TextEdit_Text(void);

#endif

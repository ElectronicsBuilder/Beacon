/*
 * ui_time_edit.h - full-screen date/time editor ("YYYY-MM-DD HH:MM") for the
 * keyboard, used by the clock, countdown and count-up modes.
 * Title on the upper line, the text on the lower line with a blinking red
 * cursor. Enter accepts a valid date/time (else "INVALID DATE"), ESC cancels.
 */
#ifndef APP_UI_TIME_EDIT_H
#define APP_UI_TIME_EDIT_H

#include "time_entry.h"

void TimeEdit_Start(const char *title, const ClockTime *initial);  /* clears the screen */
int  TimeEdit_Active(void);
void TimeEdit_Stop(void);                                          /* also on mode exit */
void TimeEdit_Tick(void);                                          /* every 1 ms */
/* TE_DONE: *result holds the valid date/time (seconds 0); the editor stays
 * open until TimeEdit_Stop(). TE_CANCEL: closed by ESC. */
TimeEntryState TimeEdit_Key(uint8_t key, ClockTime *result);

#endif

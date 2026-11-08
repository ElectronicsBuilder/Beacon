/*
 * time_entry.h - date/time editor for keyboard entry on the display.
 *
 * Text "YYYY-MM-DD HH:MM" (16 characters = 128 px in the 8 px font). Digits
 * overwrite the digit under the cursor and move right; Up/Down step the field
 * under the cursor (year, month, day, hour, minute; wrapping in its range);
 * Left/Right/Backspace move the cursor (wrapping at the ends); Enter
 * finishes, ESC cancels. Separators are skipped.
 */
#ifndef APP_TIME_ENTRY_H
#define APP_TIME_ENTRY_H

#include <stdint.h>
#include "clock.h"

#define TIME_ENTRY_LEN 16

typedef enum { TE_EDITING, TE_DONE, TE_CANCEL } TimeEntryState;

typedef struct {
  char text[TIME_ENTRY_LEN + 1];
  uint8_t pos;                     /* cursor, always on a digit */
} TimeEntry;

void TimeEntry_Start(TimeEntry *e, const ClockTime *t);
TimeEntryState TimeEntry_Key(TimeEntry *e, uint8_t key);
int TimeEntry_Get(const TimeEntry *e, ClockTime *t);   /* 0 = valid; seconds = 0 */

#endif

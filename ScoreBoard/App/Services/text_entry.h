/*
 * text_entry.h - one line of text typed on the keyboard (printable ASCII).
 * Backspace deletes, Enter finishes, ESC cancels. Pure logic (PC tests).
 */
#ifndef APP_TEXT_ENTRY_H
#define APP_TEXT_ENTRY_H

#include <stdint.h>
#include "time_entry.h"         /* TimeEntryState: TE_EDITING / TE_DONE / TE_CANCEL */

#define TEXT_ENTRY_MAX 48

typedef struct {
  char text[TEXT_ENTRY_MAX + 1];
  uint8_t len;
  uint8_t max;
} TextEntry;

void TextEntry_Start(TextEntry *e, const char *initial, uint8_t max);
TimeEntryState TextEntry_Key(TextEntry *e, uint8_t key);

#endif

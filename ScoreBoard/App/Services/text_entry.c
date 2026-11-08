/*
 * text_entry.c - one line of keyboard text. See text_entry.h.
 */
#include "text_entry.h"
#include "keys.h"
#include <string.h>

void TextEntry_Start(TextEntry *e, const char *initial, uint8_t max)
{
  e->max = (max > TEXT_ENTRY_MAX) ? TEXT_ENTRY_MAX : max;
  strncpy(e->text, initial, e->max);
  e->text[e->max] = '\0';
  e->len = (uint8_t)strlen(e->text);
}

TimeEntryState TextEntry_Key(TextEntry *e, uint8_t key)
{
  if (key == KB_ENTER || key == '\n') return TE_DONE;
  if (key == KB_ESC) return TE_CANCEL;
  if (key == KB_BACK || key == KB_DEL || key == KB_LEFT)
  {
    if (e->len > 0u) e->text[--e->len] = '\0';
  }
  else if (key >= 32u && key <= 126u && e->len < e->max)
  {
    e->text[e->len++] = (char)key;
    e->text[e->len] = '\0';
  }
  return TE_EDITING;
}

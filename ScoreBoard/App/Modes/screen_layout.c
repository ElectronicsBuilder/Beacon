/*
 * screen_layout.c - per-screen line layout. See screen_layout.h.
 */
#include "screen_layout.h"
#include "ee_record.h"
#include "matrix.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

#define LAYOUT_DEFAULT {{ { BAND_TICKER, BAND_NONE, MTX_C_BLACK }, { BAND_MSG, BAND_NONE, MTX_C_BLACK }, \
                          { BAND_BIG,    BAND_NONE, MTX_C_BLACK }, { BAND_MSG, BAND_NONE, MTX_C_BLACK } }}

static ScreenLayout layouts[SCREEN_COUNT] = { LAYOUT_DEFAULT, LAYOUT_DEFAULT, LAYOUT_DEFAULT };
static uint32_t changes;

static const char *const screen_names[SCREEN_COUNT] = { "clock", "countdown", "countup" };
static const char *const kind_names[BAND_KIND_COUNT] = { "off", "big", "text", "ticker", "msg", "time" };

static int P_Valid(const BandCfg *b)
{
  if (b->top >= BAND_KIND_COUNT || b->bg >= MTX_C_COUNT) return 0;
  if (b->bottom == BAND_NONE) return 1;
  return b->bottom < BAND_KIND_COUNT && b->top != BAND_BIG && b->bottom != BAND_BIG;
}

static void P_Save(void)
{
  changes++;
  if (EeRecord_Save(EE_RECORD_SCREEN_ADDR, layouts, sizeof layouts) != 0)
  {
    LOG_E("Screen layout: EEPROM write failed");
  }
}

void Screen_Load(void)
{
  ScreenLayout saved[SCREEN_COUNT];
  if (EeRecord_Load(EE_RECORD_SCREEN_ADDR, saved, sizeof saved) != 0) return;
  for (int s = 0; s < SCREEN_COUNT; s++)
  {
    for (int b = 0; b < SCREEN_BANDS; b++)
    {
      if (!P_Valid(&saved[s].band[b])) return;       /* not ours: keep defaults */
    }
  }
  memcpy(layouts, saved, sizeof layouts);
}

const ScreenLayout *Screen_Layout(ScreenId s)
{
  return &layouts[s < SCREEN_COUNT ? s : SCREEN_CLOCK];
}

int Screen_SetBand(ScreenId s, int band, uint8_t top, uint8_t bottom)
{
  if (s >= SCREEN_COUNT || band < 0 || band >= SCREEN_BANDS) return -1;
  BandCfg b = layouts[s].band[band];
  b.top = top;
  b.bottom = bottom;
  if (!P_Valid(&b)) return -1;
  layouts[s].band[band] = b;
  P_Save();
  return 0;
}

int Screen_SetBackground(ScreenId s, int band, uint8_t color)
{
  if (s >= SCREEN_COUNT || band < 0 || band >= SCREEN_BANDS || color >= MTX_C_COUNT) return -1;
  layouts[s].band[band].bg = color;
  P_Save();
  return 0;
}

uint32_t Screen_Changes(void)       { return changes; }
const char *Screen_Name(ScreenId s) { return s < SCREEN_COUNT ? screen_names[s] : "?"; }

int Screen_Find(const char *name)
{
  for (int i = 0; i < SCREEN_COUNT; i++)
  {
    if (strcmp(name, screen_names[i]) == 0) return i;
  }
  return -1;
}

static int P_Kind(const char *name, size_t len)
{
  for (int i = 0; i < BAND_KIND_COUNT; i++)
  {
    if (strlen(kind_names[i]) == len && strncmp(name, kind_names[i], len) == 0) return i;
  }
  return -1;
}

int Screen_ParseBand(const char *text, uint8_t *top, uint8_t *bottom)
{
  const char *slash = strchr(text, '/');
  BandCfg b = { 0, BAND_NONE, MTX_C_BLACK };
  int k = P_Kind(text, slash ? (size_t)(slash - text) : strlen(text));
  if (k < 0) return -1;
  b.top = (uint8_t)k;
  if (slash)
  {
    k = P_Kind(slash + 1, strlen(slash + 1));
    if (k < 0) return -1;
    b.bottom = (uint8_t)k;
  }
  if (!P_Valid(&b)) return -1;
  *top = b.top;
  *bottom = b.bottom;
  return 0;
}

void Screen_FormatBand(const BandCfg *b, char *text, size_t len)
{
  const char *top = b->top < BAND_KIND_COUNT ? kind_names[b->top] : "?";
  if (b->bottom == BAND_NONE)
    snprintf(text, len, "%s", top);
  else
    snprintf(text, len, "%s/%s", top, b->bottom < BAND_KIND_COUNT ? kind_names[b->bottom] : "?");
}

int Screen_BandVisible(int band)
{
#if HEIGHT == 64
  return band >= 0 && band < SCREEN_BANDS;
#else
  return band == 0 || band == 2;     /* E line not wired: lines 1 and 3 */
#endif
}

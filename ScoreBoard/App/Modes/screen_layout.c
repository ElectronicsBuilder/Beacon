/*
 * screen_layout.c - per-screen line layout. See screen_layout.h.
 */
#include "screen_layout.h"
#include "ee_record.h"
#include "matrix.h"
#include "log.h"
#include <string.h>

static ScreenLayout layouts[SCREEN_COUNT] = {
  [SCREEN_CLOCK]     = {{ BAND_TICKER, BAND_MSG, BAND_BIG, BAND_MSG }},
  [SCREEN_COUNTDOWN] = {{ BAND_TICKER, BAND_MSG, BAND_BIG, BAND_MSG }},
  [SCREEN_COUNTUP]   = {{ BAND_TICKER, BAND_MSG, BAND_BIG, BAND_MSG }},
};
static uint32_t changes;

static const char *const screen_names[SCREEN_COUNT] = { "clock", "countdown", "countup" };
static const char *const kind_names[BAND_KIND_COUNT] = { "off", "big", "text", "ticker", "msg" };

void Screen_Load(void)
{
  ScreenLayout saved[SCREEN_COUNT];
  if (EeRecord_Load(EE_RECORD_SCREEN_ADDR, saved, sizeof saved) != 0) return;
  for (int s = 0; s < SCREEN_COUNT; s++)
  {
    for (int b = 0; b < SCREEN_BANDS; b++)
    {
      if (saved[s].kind[b] >= BAND_KIND_COUNT) return;     /* not ours: keep defaults */
    }
  }
  memcpy(layouts, saved, sizeof layouts);
}

const ScreenLayout *Screen_Layout(ScreenId s)
{
  return &layouts[s < SCREEN_COUNT ? s : SCREEN_CLOCK];
}

int Screen_SetBand(ScreenId s, int band, BandKind kind)
{
  if (s >= SCREEN_COUNT || band < 0 || band >= SCREEN_BANDS || kind >= BAND_KIND_COUNT) return -1;
  if (layouts[s].kind[band] == kind) return 0;
  layouts[s].kind[band] = (uint8_t)kind;
  changes++;
  if (EeRecord_Save(EE_RECORD_SCREEN_ADDR, layouts, sizeof layouts) != 0)
  {
    LOG_E("Screen layout: EEPROM write failed");
  }
  return 0;
}

uint32_t Screen_Changes(void)           { return changes; }
const char *Screen_Name(ScreenId s)     { return s < SCREEN_COUNT ? screen_names[s] : "?"; }
const char *Screen_KindName(BandKind k) { return k < BAND_KIND_COUNT ? kind_names[k] : "?"; }

int Screen_Find(const char *name)
{
  for (int i = 0; i < SCREEN_COUNT; i++)
  {
    if (strcmp(name, screen_names[i]) == 0) return i;
  }
  return -1;
}

int Screen_KindFind(const char *name)
{
  for (int i = 0; i < BAND_KIND_COUNT; i++)
  {
    if (strcmp(name, kind_names[i]) == 0) return i;
  }
  return -1;
}

int Screen_BandVisible(int band)
{
#if HEIGHT == 64
  return band >= 0 && band < SCREEN_BANDS;
#else
  return band == 0 || band == 2;     /* E line not wired: lines 1 and 3 */
#endif
}

/*
 * screen_layout.c - per-screen line layout. See screen_layout.h.
 */
#include "screen_layout.h"
#include "ee_record.h"
#include "matrix.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LAYOUT_DEFAULT {{ { BAND_TICKER, BAND_NONE, MTX_C_BLACK }, { BAND_MSG, BAND_NONE, MTX_C_BLACK }, \
                          { BAND_BIG,    BAND_NONE, MTX_C_BLACK }, { BAND_MSG, BAND_NONE, MTX_C_BLACK } }, SCREEN_MSGS_ALL }

#define LAYOUT_END     {{ { BAND_TEXT, BAND_NONE, MTX_C_BLACK }, { BAND_MSG, BAND_NONE, MTX_C_BLACK },                           { BAND_MSG,  BAND_NONE, MTX_C_BLACK }, { BAND_MSG, BAND_NONE, MTX_C_BLACK } }, SCREEN_MSGS_ALL }

static ScreenLayout layouts[SCREEN_COUNT] = { LAYOUT_DEFAULT, LAYOUT_DEFAULT, LAYOUT_DEFAULT, LAYOUT_END };
static uint32_t changes;
static ScreenColors colors[SCREEN_COUNT] = {
  { SCREEN_COLOR_AUTO, SCREEN_COLOR_AUTO, SCREEN_COLOR_AUTO }, { SCREEN_COLOR_AUTO, SCREEN_COLOR_AUTO, SCREEN_COLOR_AUTO },
  { SCREEN_COLOR_AUTO, SCREEN_COLOR_AUTO, SCREEN_COLOR_AUTO }, { SCREEN_COLOR_AUTO, SCREEN_COLOR_AUTO, SCREEN_COLOR_AUTO },
};

static const char *const screen_names[SCREEN_COUNT] = { "clock", "countdown", "countup", "end" };
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

static int P_ColorOk(uint8_t c)
{
  return c == SCREEN_COLOR_AUTO || (c < MTX_C_COUNT && c != MTX_C_BLACK);
}

void Screen_Load(void)
{
  ScreenColors sc[SCREEN_COUNT];
  if (EeRecord_Load(EE_RECORD_SCRCOLOR_ADDR, sc, sizeof sc) == 0)
  {
    int ok = 1;
    for (int s = 0; s < SCREEN_COUNT; s++) ok &= P_ColorOk(sc[s].big) && P_ColorOk(sc[s].text) && P_ColorOk(sc[s].sand);
    if (ok) memcpy(colors, sc, sizeof colors);
  }
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

int Screen_SetMessages(ScreenId s, uint32_t mask)
{
  if (s >= SCREEN_COUNT) return -1;
  layouts[s].msg_mask = mask;
  P_Save();
  return 0;
}

int Screen_ParseMessages(int argc, char **argv, uint32_t *mask)
{
  if (argc == 1 && strcmp(argv[0], "all") == 0)  { *mask = SCREEN_MSGS_ALL; return 0; }
  if (argc == 1 && strcmp(argv[0], "none") == 0) { *mask = 0; return 0; }
  uint32_t m = 0;
  for (int i = 0; i < argc; i++)
  {
    char *end;
    long id = strtol(argv[i], &end, 10);
    if (*end != '\0' || id < 1 || id > 32) return -1;
    m |= 1uL << (id - 1);
  }
  if (argc == 0) return -1;
  *mask = m;
  return 0;
}

void Screen_FormatMessages(uint32_t mask, char *text, size_t len)
{
  if (mask == SCREEN_MSGS_ALL) { snprintf(text, len, "all"); return; }
  if (mask == 0u)              { snprintf(text, len, "none"); return; }
  size_t n = 0;
  text[0] = '\0';
  for (int i = 0; i < 32 && n + 4u < len; i++)
  {
    if (mask & (1uL << i)) n += (size_t)snprintf(text + n, len - n, "%s%d", n ? "," : "", i + 1);
  }
}

const ScreenColors *Screen_Colors(ScreenId s)
{
  return &colors[s < SCREEN_COUNT ? s : SCREEN_CLOCK];
}

int Screen_SetColor(ScreenId s, int which, uint8_t color)
{
  if (s >= SCREEN_COUNT || !P_ColorOk(color)) return -1;
  if (which == 1) colors[s].text = color;
  else if (which == 2) colors[s].sand = color;
  else colors[s].big = color;
  changes++;
  if (EeRecord_Save(EE_RECORD_SCRCOLOR_ADDR, colors, sizeof colors) != 0)
  {
    LOG_E("Screen colours: EEPROM write failed");
  }
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

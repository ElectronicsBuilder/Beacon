/*
 * mode.c - mode registry and switching.
 */
#include "mode.h"
#include "settings.h"
#include "matrix.h"
#include "usb_keyboard.h"
#include "log.h"
#include "ui_time_edit.h"
#include "ui_text_edit.h"
#include <string.h>

/* Append new games/screens at the end (the saved mode is the index). */
static const AppMode *const modes[] = {
  &mode_pingpong,
  &mode_clock,
  &mode_countdown,
  &mode_countup,
  &mode_messages,
};
#define MODE_COUNT ((int)(sizeof modes / sizeof modes[0]))

static int current = -1;

int Mode_Count(void)              { return MODE_COUNT; }
int Mode_CurrentIndex(void)       { return current; }
const AppMode *Mode_Current(void) { return current >= 0 ? modes[current] : NULL; }

const AppMode *Mode_Get(int index)
{
  return (index >= 0 && index < MODE_COUNT) ? modes[index] : NULL;
}

int Mode_Find(const char *name)
{
  for (int i = 0; i < MODE_COUNT; i++)
  {
    if (strcmp(modes[i]->name, name) == 0) return i;
  }
  return -1;
}

static void P_Enter(int index)
{
  if (current >= 0)
  {
    modes[current]->exit();
  }
  current = index;
  calculateClearImage();
  CHANGE_FONT(BLUE);
  modes[current]->enter();
  LOG_I("Mode: %s", modes[current]->name);
}

void Mode_Init(void)
{
  int saved = Settings_GetMode();
  P_Enter((saved >= 0 && saved < MODE_COUNT) ? saved : 0);
}

int Mode_Select(int index)
{
  if (index < 0 || index >= MODE_COUNT)
  {
    return -1;
  }
  if (index != current)
  {
    P_Enter(index);
    Settings_SetMode((uint8_t)index);
  }
  return 0;
}

void Mode_Tick(void)
{
  if (current >= 0)
  {
    modes[current]->tick();
  }
}

void Mode_Key(uint8_t key)
{
  if (key == 0)
  {
    return;
  }
  if (key >= KB_F1 && key <= KB_F8)
  {
    Mode_Select(key - KB_F1);
    return;
  }
  if (TimeEdit_Active() || TextEdit_Active())   /* typing: + - = _ are text, not brightness */
  {
    if (current >= 0) modes[current]->key(key);
    return;
  }
  if (key == '+' || key == '=')
  {
    Matrix_SetBrightness(Matrix_GetBrightness() + MATRIX_BRIGHTNESS_STEP);
    Settings_BrightnessChanged();
    return;
  }
  if (key == '-' || key == '_')
  {
    uint8_t b = Matrix_GetBrightness();
    Matrix_SetBrightness(b > MATRIX_BRIGHTNESS_STEP ? b - MATRIX_BRIGHTNESS_STEP : MATRIX_BRIGHTNESS_MIN);
    Settings_BrightnessChanged();
    return;
  }
  if (current >= 0)
  {
    modes[current]->key(key);
  }
}

/*
 * ui_menu.c - settings menu on the matrix. See ui_menu.h.
 */
#include "ui_menu.h"
#include "msg_store.h"
#include "settings.h"
#include "countdown.h"
#include "matrix.h"
#include "layout.h"
#include "FONT.h"
#include "keys.h"
#include "scenes.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

typedef enum { ITEM_GAP, ITEM_TIME, ITEM_TICKER, ITEM_ORDER, ITEM_CD_DAYS, ITEM_SCENE, ITEM_SCENE_S, ITEM_THEN,
               ITEM_BRIGHT, ITEM_REFRESH, ITEM_COUNT } MenuItem;

static const struct { const char *title; int lo, hi, step; } items[ITEM_COUNT] = {
  [ITEM_GAP]    = { "MESSAGE GAP",  1, 250, 1 },
  [ITEM_TIME]   = { "MESSAGE TIME", 1, 60,  1 },
  [ITEM_TICKER] = { "MSG TICKER",   0, 1,   1 },
  [ITEM_ORDER]  = { "MSG ORDER",    0, 1,   1 },
  [ITEM_SCENE]  = { "END SCENE",    0, SCENE_COUNT - 1, 1 },
  [ITEM_SCENE_S]= { "SCENE TIME",   0, 600, 5 },
  [ITEM_THEN]   = { "AFTER SCENE",  0, 1,   1 },
  [ITEM_CD_DAYS]= { "CD DAYS",      0, 1,   1 },
  [ITEM_BRIGHT] = { "BRIGHTNESS",   MATRIX_BRIGHTNESS_MIN, 100, 5 },
  [ITEM_REFRESH]= { "REFRESH",      0, 700, 10 },   /* Hz; 0 = original 2016 timing */
};

static uint8_t active;
static int item;
static MsgConfig cfg;              /* edited copies, saved on close */
static EndPlan plan;
static int typed;                  /* number being typed, -1 = none */
static uint8_t preview;            /* END SCENE: the scene is playing (P) */

static int P_Get(int i)
{
  switch (i)
  {
    case ITEM_GAP:    return cfg.gap_s;
    case ITEM_TIME:   return cfg.show_s;
    case ITEM_TICKER: return cfg.ticker;
    case ITEM_ORDER:  return cfg.order;
    case ITEM_SCENE:  return plan.scene;
    case ITEM_SCENE_S:return plan.scene_s;
    case ITEM_THEN:   return plan.then;
    case ITEM_CD_DAYS:return Settings_GetDays(0);
    case ITEM_REFRESH:return Matrix_GetBcmUnit() == 0u ? 0 : (int)Matrix_RefreshHz();
    default:          return Matrix_GetBrightness();
  }
}

static void P_Set(int i, int v)
{
  if (v < items[i].lo) v = items[i].lo;
  if (v > items[i].hi) v = items[i].hi;
  switch (i)
  {
    case ITEM_GAP:    cfg.gap_s = (uint8_t)v; break;
    case ITEM_TIME:   cfg.show_s = (uint8_t)v; break;
    case ITEM_TICKER: cfg.ticker = (uint8_t)v; break;
    case ITEM_ORDER:  cfg.order = (uint8_t)v; break;
    case ITEM_SCENE:  plan.scene = (uint8_t)v; break;
    case ITEM_SCENE_S:plan.scene_s = (uint16_t)v; break;
    case ITEM_THEN:   plan.then = (uint8_t)v; break;
    case ITEM_CD_DAYS:Settings_SetDays(0, (uint8_t)v); break;
    case ITEM_REFRESH:
      if (v == 0) Matrix_SetBcmUnit(0);             /* original timing */
      else Matrix_SetRefreshHz((uint32_t)(v < 20 ? 20 : v));
      Settings_RefreshChanged();
      break;
    default:
      Matrix_SetBrightness((uint8_t)v);            /* visible right away */
      Settings_BrightnessChanged();
      break;
  }
}

static void P_Draw(void)
{
  char title[20], value[20];
  int v = P_Get(item);
  snprintf(title, sizeof title, "%d/%d %s", item + 1, ITEM_COUNT, items[item].title);
  switch (item)
  {
    case ITEM_GAP:
    case ITEM_TIME:   snprintf(value, sizeof value, "< %d S >", typed >= 0 ? typed : v); break;
    case ITEM_TICKER: snprintf(value, sizeof value, "< %s >", v ? "ON" : "OFF"); break;
    case ITEM_ORDER:  snprintf(value, sizeof value, "< %s >", v ? "RANDOM" : "IN ORDER"); break;
    case ITEM_SCENE:  snprintf(value, sizeof value, "< %s >", EndPlan_SceneName((SceneId)v)); break;
    case ITEM_SCENE_S:
      if ((typed >= 0 ? typed : v) == 0) snprintf(value, sizeof value, "< INFINITE >");
      else snprintf(value, sizeof value, "< %d S >", typed >= 0 ? typed : v);
      break;
    case ITEM_THEN:   snprintf(value, sizeof value, "< %s >", v ? "END SCREEN" : "HOLD 0:00"); break;
    case ITEM_CD_DAYS:snprintf(value, sizeof value, "< %s >", v ? "49D LABEL" : "LABEL ONLY"); break;
    case ITEM_REFRESH:
      if (typed < 0 && v == 0) snprintf(value, sizeof value, "< ORIGINAL >");
      else snprintf(value, sizeof value, "< %d HZ >", typed >= 0 ? typed : v);
      break;
    default:          snprintf(value, sizeof value, "< %d %% >", typed >= 0 ? typed : v); break;
  }
  Matrix_SetTextColor(MTX_C_WHITE);
  calculateLEDString("                ", 0, LAYOUT_EDIT_TITLE_Y, Font8x8);
  calculateLEDString(title, LAYOUT_CENTER_X(strlen(title), 8), LAYOUT_EDIT_TITLE_Y, Font8x8);
  Matrix_SetTextColor(MTX_C_GREEN);
  calculateLEDString("                ", 0, LAYOUT_EDIT_Y, Font8x8);
  calculateLEDString(value, LAYOUT_CENTER_X(strlen(value), 8), LAYOUT_EDIT_Y, Font8x8);
#if HEIGHT == 64
  calculateLEDString("                ", 0, LAYOUT_LINE_Y(3), Font8x8);
  if (item == ITEM_SCENE && v != SCENE_NONE)
  {
    Matrix_SetTextColor(MTX_C_CYAN);
    calculateLEDString("P = PREVIEW", LAYOUT_CENTER_X(11, 8), LAYOUT_LINE_Y(3), Font8x8);
  }
#endif
  CHANGE_FONT(BLUE);
}

/*
 * Refresh rate one step up or down (10 Hz). The rates are not continuous, so if
 * the closest one is the current one, move one BCM unit instead. Below the
 * slowest rate comes the original 2016 timing, and up from it the slowest.
 */
static void P_StepRefresh(int up)
{
  uint16_t unit = Matrix_GetBcmUnit();
  if (unit == 0u)
  {
    if (up) Matrix_SetBcmUnit(MATRIX_BCM_UNIT_MAX);
  }
  else
  {
    uint32_t hz = Matrix_RefreshHz();
    if (!up && unit >= MATRIX_BCM_UNIT_MAX)
    {
      Matrix_SetBcmUnit(0);
    }
    else if (Matrix_SetRefreshHz(up ? hz + 10u : (hz > 10u ? hz - 10u : 1u)) == hz)
    {
      if (up && unit > MATRIX_BCM_UNIT_MIN) Matrix_SetBcmUnit((uint16_t)(unit - 1u));
      else if (!up && unit < MATRIX_BCM_UNIT_MAX) Matrix_SetBcmUnit((uint16_t)(unit + 1u));
    }
  }
  Settings_RefreshChanged();
}

/* finish a typed number */
static void P_Commit(void)
{
  if (typed >= 0)
  {
    P_Set(item, typed);
    typed = -1;
  }
}

void Menu_Open(void)
{
  cfg = *MsgStore_Config();
  plan = *Timer_EndPlan();
  active = 1;
  preview = 0;
  item = ITEM_GAP;
  typed = -1;
  calculateClearImage();
  P_Draw();
}

int Menu_Active(void) { return active; }

void Menu_Tick(void)
{
  if (preview) Scene_Tick();
}

static void P_StartPreview(void)
{
  calculateClearImage();
  Scene_Start((SceneId)plan.scene, EndPlan_SceneName((SceneId)plan.scene));
}

/* keys while a scene preview plays: Left/Right other scene, anything else back */
static void P_PreviewKey(uint8_t key)
{
  if (key == KB_RIGHT || key == KB_LEFT || key == '+' || key == '=' || key == '-' || key == '_')
  {
    int up = (key == KB_RIGHT || key == '+' || key == '=');
    int v = plan.scene;
    do v = (v + (up ? 1 : SCENE_COUNT - 1)) % SCENE_COUNT; while (v == SCENE_NONE);
    plan.scene = (uint8_t)v;
    P_StartPreview();
    return;
  }
  preview = 0;
  calculateClearImage();
  P_Draw();
}

int Menu_Key(uint8_t key)
{
  if (!active) return 1;
  if (preview)
  {
    P_PreviewKey(key);
    return 0;
  }
  if ((key == 'p' || key == 'P') && item == ITEM_SCENE && plan.scene != SCENE_NONE)
  {
    preview = 1;
    P_StartPreview();
    return 0;
  }
  if (key == KB_ENTER || key == KB_ESC || key == KB_F12)
  {
    P_Commit();
    active = 0;
    const MsgConfig *old = MsgStore_Config();
    if (memcmp(old, &cfg, sizeof cfg) != 0)
    {
      MsgStore_SetConfig(&cfg);
      LOG_I("Settings: gap %u s, time %u s, ticker %s, order %s", cfg.gap_s, cfg.show_s,
            cfg.ticker ? "on" : "off", cfg.order ? "random" : "seq");
    }
    if (memcmp(Timer_EndPlan(), &plan, sizeof plan) != 0)
    {
      Timer_SetEndPlan(&plan);
      LOG_I("Countdown end: %s %u s, then %s", EndPlan_SceneName((SceneId)plan.scene),
            (unsigned)plan.scene_s, EndPlan_ThenName((EndThen)plan.then));
    }
    calculateClearImage();
    return 1;
  }
  if (key >= '0' && key <= '9' && items[item].hi > 1 && item != ITEM_SCENE)
  {
    int t = (typed < 0 ? 0 : typed) * 10 + (key - '0');
    typed = (t > items[item].hi) ? key - '0' : t;     /* too big: start again */
  }
  else if (key == KB_BACK && typed >= 0)
  {
    typed = (typed >= 10) ? typed / 10 : -1;
  }
  else if (key == KB_UP || key == KB_DOWN)
  {
    P_Commit();
    item = (item + (key == KB_DOWN ? 1 : ITEM_COUNT - 1)) % ITEM_COUNT;
  }
  else if (key == KB_RIGHT || key == KB_LEFT || key == '+' || key == '=' || key == '-' || key == '_')
  {
    P_Commit();
    int up = (key == KB_RIGHT || key == '+' || key == '=');
    int v = P_Get(item);
    if (item == ITEM_REFRESH) { P_StepRefresh(up); P_Draw(); return 0; }
    if (items[item].hi == 1) v = !v;                   /* on/off, order: toggle */
    else if (item == ITEM_SCENE) v = (v + (up ? 1 : SCENE_COUNT - 1)) % SCENE_COUNT;   /* wraps */
    else if (item == ITEM_SCENE_S && !up && v == 0) v = items[item].hi;          /* below infinite: wrap */
    else v += up ? items[item].step : -items[item].step;
    P_Set(item, v);
  }
  P_Draw();
  return 0;
}

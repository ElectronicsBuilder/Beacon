/*
 * mode_pingpong.c - the 2016 ping-pong scoreboard as a mode.
 *
 * The game logic is unchanged (Game/GAME.c, Timers/TIM_CTRL.c): its periodic
 * jobs (game clock, scrolling text, display update) run only while this mode
 * is active. The game state is kept when another mode is selected.
 */
#include "mode.h"
#include "app_timers.h"
#include "TIM_CTRL.h"
#include "GAME.h"

static uint8_t active;

int PingPong_IsActive(void)
{
  return active;
}

static void P_Enter(void)
{
  active = 1;
  SLEEP_STAT = sleep_display_disable;
  SCREEN_SLEEP = 0;
  SCREEN_SLEEP_MIN = 0;
  RESUME_UPDATE_DISPLAY();     /* game clock, scrolling text, display update */
}

static void P_Exit(void)
{
  STOP_UPDATE_DISPLAY();
  active = 0;
}

static void P_Tick(void)
{
  AppTimers_Run();
  /* The 2016 keyboard job called the key handler every millisecond, also
   * without a key, which keeps the score at 0:0 outside a game. Same here. */
  if (GAME.STATUS != start)
  {
    RESET_SCORE();
  }
}

static void P_Key(uint8_t key)
{
  if (key < 0x80)              /* the game uses ASCII keys only */
  {
    KEYBOARD_CONTROL(key);
  }
}

const AppMode mode_pingpong = {
  "pingpong", "Ping-pong", MODE_KIND_GAME, P_Enter, P_Exit, P_Tick, P_Key
};

/*
 * cmd_game.c - ping-pong game commands: players, statistics and game control
 * (select, start, points). Control uses the same key handler as the USB
 * keyboard, so the game behaves exactly as with the keyboard.
 */
#include "commands.h"
#include "cli.h"
#include "GAME.h"
#include "TIM_CTRL.h"
#include "mode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_COUNT 11

/* game players: id and name of every stored player */
static int P_Players(void)
{
  for (int id = 0; id < PLAYER_COUNT; id++)
  {
    Cli_Printf("player=%d name=%s", id, GAMER_ALL[id].name);
  }
  return 0;
}

/* Print one row of per-opponent values as "a,b,c,..." */
static void P_Row(const char *key, const char *values)
{
  char text[PLAYER_COUNT * 4 + 1];
  size_t n = 0;
  for (int i = 0; i < PLAYER_COUNT && n < sizeof text - 4; i++)
  {
    n += (size_t)snprintf(text + n, sizeof text - n, "%s%u", i ? "," : "", (unsigned)(uint8_t)values[i]);
  }
  Cli_Printf("%s=%s", key, text);
}

/* game stats <id>: name, message, wins and best score against each opponent */
static int P_Stats(const char *arg)
{
  char *end;
  long id = strtol(arg, &end, 10);
  if (*end != '\0' || id < 0 || id >= PLAYER_COUNT)
  {
    return Cli_Error("player id must be 0..%d", PLAYER_COUNT - 1);
  }
  const GAMERS *g = &GAMER_ALL[id];
  Cli_Printf("player=%ld", id);
  Cli_Printf("name=%s", g->name);
  Cli_Printf("message=%s", g->message);
  P_Row("wins_vs", g->PLAYER_DATA);         /* games won against player 0..10 */
  P_Row("best_vs", g->SAVED_SCORE_DATA);    /* best score against player 0..10 */
  return 0;
}

extern uint16_t id, id2;            /* next player for side 1 / 2 (TIM_CTRL.c) */

static int P_PlayerId(const char *arg, long *out)
{
  char *end;
  long v = strtol(arg, &end, 10);
  if (*end != '\0' || v < 0 || v >= PLAYER_COUNT) return -1;
  *out = v;
  return 0;
}

static int P_NeedPingPong(void)
{
  if (!PingPong_IsActive()) return Cli_Error("ping-pong is not shown (mode pingpong)");
  return 0;
}

static void P_Score(void)
{
  Cli_Printf("state=%s", GAME.STATUS == start ? "playing" : "idle");
  Cli_Printf("player1=%u %s", (unsigned)PLAYER1.id, PLAYER1.name);
  Cli_Printf("player2=%u %s", (unsigned)PLAYER2.id, PLAYER2.name);
  Cli_Printf("score=%u:%u", (unsigned)PLAYER1.score, (unsigned)PLAYER2.score);
}

/* game select / start / point / undo */
static int P_Control(int argc, char **argv)
{
  long a, b;
  if (P_NeedPingPong() != 0) return -1;
  if (strcmp(argv[1], "select") == 0 && argc == 4)
  {
    if (GAME.STATUS == start) return Cli_Error("a game is running");
    if (P_PlayerId(argv[2], &a) != 0 || P_PlayerId(argv[3], &b) != 0)
      return Cli_Error("player ids 0..%d (game players)", PLAYER_COUNT - 1);
    if (a == b) return Cli_Error("two different players");
    id = (uint16_t)a;
    KEYBOARD_CONTROL('a');               /* = keyboard 'a': selects GAMER_ALL[id] for side 1 */
    id2 = (uint16_t)b;
    KEYBOARD_CONTROL('k');
  }
  else if (strcmp(argv[1], "start") == 0 && argc == 2)
  {
    KEYBOARD_CONTROL('s');
    if (GAME.STATUS != start) return Cli_Error("not started (same player on both sides?)");
  }
  else if ((strcmp(argv[1], "point") == 0 || strcmp(argv[1], "undo") == 0) && argc == 3)
  {
    if (GAME.STATUS != start) return Cli_Error("no game running (game start)");
    int side = argv[2][0] == '1' ? 1 : argv[2][0] == '2' ? 2 : 0;
    if (side == 0 || argv[2][1] != '\0') return Cli_Error("side 1 or 2");
    int undo = argv[1][0] == 'u';
    KEYBOARD_CONTROL(side == 1 ? (undo ? 'z' : 'a') : (undo ? 'm' : 'k'));
  }
  else
  {
    return Cli_Error("usage: game select <p1> <p2> | start | point 1|2 | undo 1|2");
  }
  P_Score();
  return 0;
}

/* game players | game stats <id> | control */
static int P_Game(int argc, char **argv)
{
  if (argc == 1)
  {
    P_Score();
    return 0;
  }
  if (argc >= 2 && strcmp(argv[1], "players") == 0)
  {
    return P_Players();
  }
  if (strcmp(argv[1], "select") == 0 || strcmp(argv[1], "start") == 0 ||
      strcmp(argv[1], "point") == 0 || strcmp(argv[1], "undo") == 0)
  {
    return P_Control(argc, argv);
  }
  if (argc >= 3 && strcmp(argv[1], "stats") == 0)
  {
    return P_Stats(argv[2]);
  }
  return Cli_Error("use: game [players | stats <id> | select <p1> <p2> | start | point 1|2 | undo 1|2]");
}

static const CliCommand game_commands[] = {
  { "game", "[players|stats|select|...]", "ping-pong: players, records, control", P_Game,
    "game                     state, players and score\n"
    "game players             the 11 players (id, name)\n"
    "game stats <id>          a player's record: wins and best score against each other one\n"
    "game select <p1> <p2>    choose the players (ids from game players)\n"
    "game start               start the game (random server)\n"
    "game point 1|2           a point for side 1 / 2;  game undo 1|2  take one back\n"
    "The game must be shown (mode pingpong).\n"
    "Playing: keyboard a / k select players and add points, z / m remove, s start\n"
    "(help keys), or the console 'keys' mode." },
};

void CmdGame_Register(void)
{
  Cli_Register(game_commands, sizeof game_commands / sizeof game_commands[0]);
}

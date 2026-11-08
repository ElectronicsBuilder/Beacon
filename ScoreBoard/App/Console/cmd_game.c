/*
 * cmd_game.c - ping-pong game commands (player list and statistics).
 * More game control (select/start/score) comes with the mode framework (M2).
 */
#include "commands.h"
#include "cli.h"
#include "GAME.h"
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

/* game players | game stats <id> */
static int P_Game(int argc, char **argv)
{
  if (argc >= 2 && strcmp(argv[1], "players") == 0)
  {
    return P_Players();
  }
  if (argc >= 3 && strcmp(argv[1], "stats") == 0)
  {
    return P_Stats(argv[2]);
  }
  return Cli_Error("use: game players | game stats <id>");
}

static const CliCommand game_commands[] = {
  { "game", "players | stats <id>", "list players, show a player's record", P_Game },
};

void CmdGame_Register(void)
{
  Cli_Register(game_commands, sizeof game_commands / sizeof game_commands[0]);
}

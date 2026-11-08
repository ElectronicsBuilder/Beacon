/*
 * mode.h - display modes (games and other screens) of the scoreboard.
 *
 * Exactly one mode owns the LED matrix at a time. The App task calls
 * Mode_Tick() every millisecond and Mode_Key() for every key from the USB
 * keyboard (and the console "keys" mode outside ping-pong).
 *
 * Keys handled for every mode:  F1..F8 select mode 1..8, '+' / '-' brightness.
 *
 * Adding a game or screen: write mode_<name>.c with an AppMode, then append
 * it to the table in mode.c (append only: the saved mode is the table index).
 */
#ifndef APP_MODE_H
#define APP_MODE_H

#include <stdint.h>

typedef enum { MODE_KIND_GAME, MODE_KIND_DISPLAY } ModeKind;

typedef struct {
  const char *name;            /* console name, e.g. "pingpong" */
  const char *title;           /* human readable */
  ModeKind kind;
  void (*enter)(void);         /* takes over the matrix (clear and draw) */
  void (*exit)(void);          /* stops its periodic work */
  void (*tick)(void);          /* every 1 ms while active */
  void (*key)(uint8_t key);    /* keyboard key (ASCII or KB_* code) */
} AppMode;

void Mode_Init(void);                      /* enters the saved mode (or the first one) */
int  Mode_Count(void);
const AppMode *Mode_Get(int index);
int  Mode_Find(const char *name);          /* index or -1 */
int  Mode_CurrentIndex(void);
const AppMode *Mode_Current(void);
int  Mode_Select(int index);               /* 0 = ok; switch and save */
void Mode_Tick(void);
void Mode_Key(uint8_t key);

/* the modes (mode_*.c) */
extern const AppMode mode_pingpong;
extern const AppMode mode_clock;

/* 1 while the ping-pong game owns the display (legacy radio/serial input is
 * only used then) */
int PingPong_IsActive(void);

#endif

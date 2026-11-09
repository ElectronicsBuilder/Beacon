/*
 * cli.h - command line console on USART3 (and later BLE).
 *
 * Human mode (default): characters are echoed, Backspace edits, Ctrl-C clears
 * the line, Up-arrow recalls the last command, a "> " prompt is shown.
 * Machine mode ("echo off"): no echo and no prompt, for scripts and the GUI.
 * Every command ends with a line "OK" or "ERR <reason>"; data lines come
 * before it, preferably as "key=value".
 *
 * Legacy 2016 protocol on the same port:
 *  - a line starting with '*' or '<' is a frame (e.g. "*gmHELLO#") and goes to
 *    the original frame parser;
 *  - the "keys" command switches to single-key control (a/b/k/m, '/', '|'...)
 *    until ESC is pressed.
 */
#ifndef APP_CLI_H
#define APP_CLI_H

#include <stddef.h>

#define CLI_MAX_ARGS 12

typedef int (*CliHandler)(int argc, char **argv);   /* return 0 = OK, else ERR */

typedef struct {
  const char *name;    /* command word, e.g. "time" */
  const char *args;    /* argument summary for help, e.g. "[YYYY-MM-DD] [HH:MM:SS]" */
  const char *help;    /* one-line description */
  CliHandler  run;
  const char *usage;   /* "help <command>": details and examples, lines split by \n */
} CliCommand;

void Cli_Init(void);
void Cli_Input(char c);                 /* one received byte (App task) */
int  Cli_Register(const CliCommand *table, size_t count);

/* For command handlers */
void Cli_Printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));  /* one output line */
int  Cli_Error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));   /* set ERR reason, returns -1 */
int  Cli_IsMachine(void);
void Cli_SetMachine(int on);
void Cli_EnterKeysMode(void);

/* Used by log.c so log lines don't break a half-typed command line */
void Cli_BeginAsyncOutput(void);
void Cli_EndAsyncOutput(void);

#endif

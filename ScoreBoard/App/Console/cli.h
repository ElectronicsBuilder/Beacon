/*
 * cli.h - command line console on USART3 (USB serial) and BLE (HM-11, UART7).
 *
 * Each port has its own typing state:
 * Human mode (default): characters are echoed, Backspace edits, Ctrl-C clears
 * the line, Up-arrow recalls the last command, a "> " prompt is shown.
 * Machine mode ("echo off"): no echo and no prompt, for scripts and the GUI.
 * Every command ends with a line "OK" or "ERR <reason>"; data lines come
 * before it, preferably as "key=value". Replies go to the port the command
 * came from; log lines go to every port in human mode.
 *
 * Legacy 2016 protocol on the same ports:
 *  - a line starting with '*' or '<' is a frame (e.g. "*gmHELLO#") and goes to
 *    the original frame parser;
 *  - the "keys" command switches to single-key control until ESC.
 */
#ifndef APP_CLI_H
#define APP_CLI_H

#include <stddef.h>

#define CLI_MAX_ARGS 12

typedef enum { CLI_PORT_SERIAL, CLI_PORT_BLE, CLI_PORT_COUNT } CliPort;

typedef int (*CliHandler)(int argc, char **argv);   /* return 0 = OK, else ERR */

typedef struct {
  const char *name;    /* command word, e.g. "time" */
  const char *args;    /* argument summary for help, e.g. "[YYYY-MM-DD] [HH:MM:SS]" */
  const char *help;    /* one-line description */
  CliHandler  run;
  const char *usage;   /* "help <command>": details and examples, lines split by \n */
} CliCommand;

/* how a port sends text; NULL / not connected = output dropped */
typedef void (*CliWriter)(const char *s, size_t len);

void Cli_Init(void);
void Cli_SetWriter(CliPort port, CliWriter writer);
void Cli_Input(char c);                          /* USB serial byte (App task) */
void Cli_InputFrom(CliPort port, char c);        /* byte from a port (App task) */
void Cli_PortConnected(CliPort port);            /* e.g. BLE link up: greeting + prompt */
int  Cli_Register(const CliCommand *table, size_t count);

/* For command handlers (act on the port the command came from) */
void Cli_Printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));  /* one output line */
int  Cli_Error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));   /* set ERR reason, returns -1 */
int  Cli_IsMachine(void);
void Cli_SetMachine(int on);
void Cli_EnterKeysMode(void);
CliPort Cli_CurrentPort(void);

/* Used by log.c: one log line to every port in human mode, keeping a
 * half-typed command line intact */
void Cli_Log(const char *line);

#endif

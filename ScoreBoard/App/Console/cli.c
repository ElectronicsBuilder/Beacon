/*
 * cli.c - command line console on USART3. See cli.h.
 */
#include "cli.h"
#include "Ez_USART.h"
#include "Comm.h"
#include "cli_parse.h"
#include "mode.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define CLI_LINE_MAX   160
#define CLI_MAX_TABLES 8
#define CLI_PROMPT     "> "

typedef enum { IN_LINE, IN_FRAME, IN_KEYS } InputMode;

static const CliCommand *tables[CLI_MAX_TABLES];
static size_t table_sizes[CLI_MAX_TABLES];
static size_t table_count;

static char line[CLI_LINE_MAX];
static size_t line_len;
static char last_line[CLI_LINE_MAX];   /* Up-arrow recall */
static InputMode input_mode = IN_LINE;
static int machine;                    /* 1 = no echo / prompt */
static int esc_state;                  /* 0, 1 = ESC seen, 2 = ESC [ seen */
static char prev_char;
static int frame_for_game;             /* legacy frame goes to the ping-pong game */
static char error_text[96];
static int executing;                  /* a command is running: no prompt redraw */

static void P_Write(const char *s)
{
  USART_puts(USART3, (char *)s);
}

static void P_Prompt(void)
{
  if (!machine && input_mode == IN_LINE && !executing)
  {
    P_Write(CLI_PROMPT);
    line[line_len] = '\0';
    P_Write(line);
  }
}

int Cli_Register(const CliCommand *table, size_t count)
{
  if (table_count >= CLI_MAX_TABLES) return -1;
  tables[table_count] = table;
  table_sizes[table_count] = count;
  table_count++;
  return 0;
}

void Cli_Printf(const char *fmt, ...)
{
  char buf[192];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  P_Write(buf);
  P_Write("\r\n");
}

int Cli_Error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(error_text, sizeof error_text, fmt, ap);
  va_end(ap);
  return -1;
}

int Cli_IsMachine(void)      { return machine; }
void Cli_SetMachine(int on)  { machine = on ? 1 : 0; }

void Cli_EnterKeysMode(void)
{
  input_mode = IN_KEYS;
}

void Cli_BeginAsyncOutput(void)
{
  if (!machine && input_mode == IN_LINE && !executing)
  {
    P_Write("\r\x1b[K");      /* clear the prompt line */
  }
}

void Cli_EndAsyncOutput(void)
{
  P_Prompt();                 /* redraw prompt + partly typed command */
}

static const CliCommand *P_Find(const char *name)
{
  for (size_t t = 0; t < table_count; t++)
  {
    for (size_t i = 0; i < table_sizes[t]; i++)
    {
      if (strcmp(tables[t][i].name, name) == 0) return &tables[t][i];
    }
  }
  return NULL;
}

static void P_Execute(const char *command_line)
{
  static char text[CLI_LINE_MAX];   /* tokenized copy; 'line' is reused for the next input */
  char *argv[CLI_MAX_ARGS];
  strlcpy(text, command_line, sizeof text);
  int argc = Cli_Tokenize(text, argv, CLI_MAX_ARGS);
  if (argc == 0) return;

  const CliCommand *cmd = P_Find(argv[0]);
  error_text[0] = '\0';
  if (cmd == NULL)
  {
    Cli_Printf("ERR unknown command '%s' (type help)", argv[0]);
    return;
  }
  executing = 1;
  int rc = cmd->run(argc, argv);
  executing = 0;
  if (rc == 0)
  {
    Cli_Printf("OK");
  }
  else
  {
    Cli_Printf("ERR %s", error_text[0] ? error_text : "failed");
  }
}

/* help [command]: list all commands or show one */
static int P_CmdHelp(int argc, char **argv)
{
  if (argc > 1)
  {
    const CliCommand *cmd = P_Find(argv[1]);
    if (cmd == NULL) return Cli_Error("unknown command '%s'", argv[1]);
    Cli_Printf("%s %s", cmd->name, cmd->args);
    Cli_Printf("  %s", cmd->help);
    return 0;
  }
  for (size_t t = 0; t < table_count; t++)
  {
    for (size_t i = 0; i < table_sizes[t]; i++)
    {
      char left[48];
      snprintf(left, sizeof left, "%s %s", tables[t][i].name, tables[t][i].args);
      Cli_Printf("  %-34s %s", left, tables[t][i].help);
    }
  }
  Cli_Printf("  Lines starting with '*' or '<' are legacy frames, e.g. *gmHELLO#");
  return 0;
}

static const CliCommand cli_builtin[] = {
  { "help", "[command]", "list commands or show one command", P_CmdHelp },
};

void Cli_Init(void)
{
  Cli_Register(cli_builtin, sizeof cli_builtin / sizeof cli_builtin[0]);
  if (!machine)
  {
    P_Write("\r\nType 'help' for commands.\r\n");
    P_Prompt();
  }
}

static void P_Backspace(void)
{
  if (line_len > 0)
  {
    line_len--;
    if (!machine) P_Write("\b \b");
  }
}

void Cli_Input(char c)
{
  /* Single-key mode: every key goes to the original serial key handler */
  if (input_mode == IN_KEYS)
  {
    if (c == 0x1B)   /* ESC */
    {
      input_mode = IN_LINE;
      P_Write("\r\n[console]\r\n");
      P_Prompt();
    }
    else if (PingPong_IsActive())
    {
      APP_USART3_Byte(c);      /* 2016 serial keys */
    }
    else
    {
      Mode_Key((uint8_t)c);    /* keys for the current mode */
    }
    return;
  }

  /* Legacy frame: forward until the end character (ping-pong mode only) */
  if (input_mode == IN_FRAME)
  {
    if (frame_for_game) COMMAND_RECEIVER2(c);
    if (!machine && c != '\r' && c != '\n') USART_putc(USART3, c);
    if (c == '#' || c == '>')
    {
      input_mode = IN_LINE;
      if (!machine) P_Write("\r\n");
      Cli_Printf(frame_for_game ? "OK" : "ERR ping-pong mode not active");
      P_Prompt();
    }
    return;
  }

  /* ANSI escape sequences (arrow keys) */
  if (esc_state == 1) { esc_state = (c == '[') ? 2 : 0; return; }
  if (esc_state == 2)
  {
    esc_state = 0;
    if (c == 'A' && last_line[0] != '\0')   /* Up: recall last command */
    {
      while (line_len > 0) P_Backspace();
      line_len = strlen(last_line);
      memcpy(line, last_line, line_len + 1u);
      if (!machine) P_Write(line);
    }
    return;
  }

  switch (c)
  {
    case 0x1B:                        /* ESC */
      esc_state = 1;
      break;

    case '\r':
    case '\n':
      if (c == '\n' && prev_char == '\r') break;   /* CR LF counts once */
      if (!machine) P_Write("\r\n");
      line[line_len] = '\0';
      if (line_len > 0)
      {
        memcpy(last_line, line, line_len + 1u);
        line_len = 0;
        P_Execute(last_line);
      }
      P_Prompt();
      break;

    case '\b':
    case 0x7F:                        /* Backspace / Delete */
      P_Backspace();
      break;

    case 0x03:                        /* Ctrl-C */
      line_len = 0;
      if (!machine) P_Write("^C\r\n");
      P_Prompt();
      break;

    default:
      if ((c == '*' || c == '<') && line_len == 0)   /* legacy frame starts */
      {
        input_mode = IN_FRAME;
        frame_for_game = PingPong_IsActive();
        if (frame_for_game) COMMAND_RECEIVER2(c);
        if (!machine) USART_putc(USART3, c);
      }
      else if (c >= 0x20 && c < 0x7F && line_len < CLI_LINE_MAX - 1u)
      {
        line[line_len++] = c;
        if (!machine) USART_putc(USART3, c);
      }
      break;
  }
  prev_char = c;
}

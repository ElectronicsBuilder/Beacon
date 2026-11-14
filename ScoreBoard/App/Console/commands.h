/*
 * commands.h - console command sets (registered with the CLI at start-up).
 */
#ifndef APP_COMMANDS_H
#define APP_COMMANDS_H

void CmdSystem_Register(void);   /* info, status, log, bright, time, echo, keys, reboot */
void CmdGame_Register(void);
void CmdTimer_Register(void);
void CmdMsg_Register(void);
void CmdBle_Register(void);
void CmdFlash_Register(void);     /* game players, game stats */

#endif

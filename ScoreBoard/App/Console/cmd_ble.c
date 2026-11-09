/*
 * cmd_ble.c - console commands for the HM-11 Bluetooth module.
 */
#include "commands.h"
#include "cli.h"
#include "log.h"
#include "hm11.h"
#include <string.h>

static void P_Show(void)
{
  Cli_Printf("ble=%s", Ble_StateName());
  if (Ble_State() != BLE_ABSENT)
  {
    Cli_Printf("name=%s", Ble_Name());
    Cli_Printf("pin=%s", Ble_Pin()[0] ? Ble_Pin() : "none");
    Cli_Printf("baud=%lu", (unsigned long)Ble_Baud());
  }
}

static int P_Ble(int argc, char **argv)
{
  if (argc >= 2 && strcmp(argv[1], "status") != 0)
  {
    if (Ble_State() == BLE_CONNECTED)
      return Cli_Error("disconnect Bluetooth first (the module takes settings only when idle)");
    if (Ble_State() == BLE_ABSENT)
      return Cli_Error("no HM-11 module answered at boot");
    if (strcmp(argv[1], "name") == 0 && argc == 3)
    {
      if (Ble_SetName(argv[2]) != 0) return Cli_Error("name: 1..12 letters, digits, - or _");
      LOG_I("BLE name set to %s", argv[2]);
    }
    else if (strcmp(argv[1], "pin") == 0 && argc == 3)
    {
      int off = strcmp(argv[2], "off") == 0;
      if (Ble_SetPin(off ? NULL : argv[2]) != 0) return Cli_Error("pin: 6 digits, or off");
      LOG_I("BLE PIN %s", off ? "off" : "set");
    }
    else
    {
      return Cli_Error("usage: ble [status | name <name> | pin <6 digits>|off]");
    }
  }
  P_Show();
  return 0;
}

static const CliCommand ble_commands[] = {
  { "ble", "[status|name|pin]", "Bluetooth (HM-11) console", P_Ble,
    "ble                 state (no module / waiting / connected), name, PIN, baud\n"
    "ble name ScoreBoard name shown when scanning (1..12 letters, digits, - _)\n"
    "ble pin 123456      require this PIN when pairing;  ble pin off  no PIN\n"
    "Name and PIN can only be changed while nothing is connected over Bluetooth\n"
    "(use the USB serial console). Connected phones/PCs get the same console:\n"
    "e.g. nRF Connect or 'Serial Bluetooth Terminal', then type help." },
};

void CmdBle_Register(void)
{
  Cli_Register(ble_commands, sizeof ble_commands / sizeof ble_commands[0]);
}

/*
 * cmd_flash.c - console commands for the SPI flash (W25Q16JV).
 */
#include "commands.h"
#include "cli.h"
#include "log.h"
#include "w25q.h"
#include <string.h>

static void P_Show(void)
{
  if (!Flash_Present())
  {
    Cli_Printf("flash=none");
    return;
  }
  Cli_Printf("flash=ok");
  Cli_Printf("jedec=%06lX", (unsigned long)Flash_Id());
  Cli_Printf("size_kb=%lu", (unsigned long)(Flash_Size() / 1024u));
  Cli_Printf("cs=%s", Flash_CsName());
}

/* erase / program / read back the last 4 KB sector (not used for data) */
static int P_Test(void)
{
  static uint8_t out[W25Q_PAGE], in[W25Q_PAGE];
  uint32_t addr = Flash_Size() - W25Q_SECTOR;
  for (unsigned i = 0; i < W25Q_PAGE; i++) out[i] = (uint8_t)(i * 7u + 3u);
  if (Flash_EraseSector(addr) != 0) return Cli_Error("erase failed");
  if (Flash_Read(addr, in, sizeof in) != 0) return Cli_Error("read failed");
  for (unsigned i = 0; i < W25Q_PAGE; i++)
    if (in[i] != 0xFFu) return Cli_Error("not blank after erase (byte %u = %02X)", i, in[i]);
  if (Flash_Write(addr, out, sizeof out) != 0) return Cli_Error("program failed");
  if (Flash_Read(addr, in, sizeof in) != 0) return Cli_Error("read failed");
  if (memcmp(in, out, sizeof in) != 0) return Cli_Error("read back differs");
  Flash_EraseSector(addr);
  Cli_Printf("test=passed (erase, program, read back at %06lX)", (unsigned long)addr);
  LOG_I("Flash test passed");
  return 0;
}

static int P_Flash(int argc, char **argv)
{
  if (argc >= 2 && strcmp(argv[1], "test") == 0)
  {
    if (!Flash_Present()) return Cli_Error("no flash chip found");
    return P_Test();
  }
  if (argc >= 2 && strcmp(argv[1], "id") == 0)
  {
    Flash_Init();                       /* probe again */
  }
  else if (argc >= 2)
  {
    return Cli_Error("usage: flash [id | test]");
  }
  P_Show();
  return 0;
}

static const CliCommand flash_commands[] = {
  { "flash", "[id|test]", "SPI flash chip (W25Q16, 2 MB)", P_Flash,
    "flash        state, JEDEC ID, size and which chip select it is on\n"
    "flash id     probe the chip again\n"
    "flash test   erase, program and read back the last 4 KB sector" },
};

void CmdFlash_Register(void)
{
  Cli_Register(flash_commands, sizeof flash_commands / sizeof flash_commands[0]);
}

/*
 * w25q.c - W25Q16JV SPI NOR flash driver. See w25q.h.
 */
#include "w25q.h"
#include "app_hal.h"
#include "log.h"
#include "cmsis_os.h"

#define CMD_WRITE_ENABLE  0x06u
#define CMD_READ_STATUS1  0x05u
#define CMD_READ_DATA     0x03u
#define CMD_PAGE_PROGRAM  0x02u
#define CMD_SECTOR_ERASE  0x20u
#define CMD_JEDEC_ID      0x9Fu
#define CMD_RELEASE_PD    0xABu
#define STATUS_BUSY       0x01u
#define SPI_TIMEOUT_MS    100u

static GPIO_TypeDef *cs_port;
static uint16_t cs_pin;
static const char *cs_name = "none";
static uint32_t jedec;

static void P_Select(void)   { HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET); }
static void P_Deselect(void) { HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET); }

static int P_Tx(const uint8_t *d, uint16_t n)
{
  return HAL_SPI_Transmit(&hspi4, (uint8_t *)d, n, SPI_TIMEOUT_MS) == HAL_OK ? 0 : -1;
}

static int P_Rx(uint8_t *d, uint16_t n)
{
  return HAL_SPI_Receive(&hspi4, d, n, SPI_TIMEOUT_MS) == HAL_OK ? 0 : -1;
}

static void P_Cmd(uint8_t cmd)
{
  P_Select();
  P_Tx(&cmd, 1);
  P_Deselect();
}

static uint32_t P_ReadId(void)
{
  uint8_t cmd = CMD_JEDEC_ID, id[3] = { 0, 0, 0 };
  P_Select();
  P_Tx(&cmd, 1);
  P_Rx(id, 3);
  P_Deselect();
  return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

static int P_WaitReady(uint32_t timeout_ms)
{
  uint8_t cmd = CMD_READ_STATUS1, st = STATUS_BUSY;
  uint32_t t0 = osKernelGetTickCount();
  do
  {
    P_Select();
    P_Tx(&cmd, 1);
    P_Rx(&st, 1);
    P_Deselect();
    if ((st & STATUS_BUSY) == 0u) return 0;
    osDelay(1);
  } while (osKernelGetTickCount() - t0 < timeout_ms);
  return -1;
}

/* a real chip answers with a manufacturer byte that is neither 00 nor FF */
static int P_IdValid(uint32_t id)
{
  uint8_t m = (uint8_t)(id >> 16);
  return m != 0x00u && m != 0xFFu;
}

int Flash_Init(void)
{
  static const struct { GPIO_TypeDef *port; uint16_t pin; const char *name; } cs[] = {
    { FLASH_CE_GPIO_Port, FLASH_CE_Pin, "FLASH_CE (PE15)" },
    { SPI4_CS_GPIO_Port,  SPI4_CS_Pin,  "SPI4_CS (PE11)" },
  };
  HAL_GPIO_WritePin(FLASH_WP_GPIO_Port, FLASH_WP_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(FLASH_HOLD_GPIO_Port, FLASH_HOLD_Pin, GPIO_PIN_SET);
  jedec = 0;
  for (size_t i = 0; i < sizeof cs / sizeof cs[0]; i++)
  {
    cs_port = cs[i].port;
    cs_pin = cs[i].pin;
    P_Deselect();
    P_Cmd(CMD_RELEASE_PD);                 /* wake it if it was powered down */
    osDelay(1);
    uint32_t id = P_ReadId();
    if (P_IdValid(id))
    {
      jedec = id;
      cs_name = cs[i].name;
      LOG_I("Flash: JEDEC %06lX on %s, %lu KB", (unsigned long)id, cs_name,
            (unsigned long)(Flash_Size() / 1024u));
      return 0;
    }
  }
  cs_name = "none";
  LOG_W("Flash: no SPI flash answered (JEDEC ID 00/FF on both chip selects)");
  return -1;
}

int Flash_Present(void)        { return jedec != 0u; }
uint32_t Flash_Id(void)        { return jedec; }
const char *Flash_CsName(void) { return cs_name; }

uint32_t Flash_Size(void)
{
  uint8_t cap = (uint8_t)jedec;          /* 0x15 = 2^21 bytes for the W25Q16 */
  return (jedec != 0u && cap >= 0x10u && cap <= 0x20u) ? (1uL << cap) : 0u;
}

int Flash_Read(uint32_t addr, void *buf, size_t len)
{
  if (!Flash_Present() || addr + len > Flash_Size()) return -1;
  uint8_t hdr[4] = { CMD_READ_DATA, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
  int rc = 0;
  P_Select();
  rc |= P_Tx(hdr, 4);
  uint8_t *p = (uint8_t *)buf;
  while (len > 0u && rc == 0)
  {
    uint16_t n = len > 0x8000u ? 0x8000u : (uint16_t)len;
    rc |= P_Rx(p, n);
    p += n;
    len -= n;
  }
  P_Deselect();
  return rc;
}

int Flash_Write(uint32_t addr, const void *buf, size_t len)
{
  if (!Flash_Present() || addr + len > Flash_Size()) return -1;
  const uint8_t *p = (const uint8_t *)buf;
  while (len > 0u)
  {
    size_t n = W25Q_PAGE - (addr % W25Q_PAGE);          /* up to the page end */
    if (n > len) n = len;
    uint8_t hdr[4] = { CMD_PAGE_PROGRAM, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
    P_Cmd(CMD_WRITE_ENABLE);
    P_Select();
    int rc = P_Tx(hdr, 4) | P_Tx(p, (uint16_t)n);
    P_Deselect();
    if (rc != 0 || P_WaitReady(10u) != 0) return -1;      /* page program: max 3 ms */
    addr += n;
    p += n;
    len -= n;
  }
  return 0;
}

int Flash_EraseSector(uint32_t addr)
{
  if (!Flash_Present() || addr >= Flash_Size()) return -1;
  addr &= ~(W25Q_SECTOR - 1u);
  uint8_t hdr[4] = { CMD_SECTOR_ERASE, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
  P_Cmd(CMD_WRITE_ENABLE);
  P_Select();
  int rc = P_Tx(hdr, 4);
  P_Deselect();
  return (rc == 0 && P_WaitReady(500u) == 0) ? 0 : -1;    /* sector erase: max 400 ms */
}

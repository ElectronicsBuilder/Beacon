/*
 * hm11.c - HM-11 BLE module as a console port. See hm11.h.
 */
#include "hm11.h"
#include "app_hal.h"
#include "cli.h"
#include "log.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

#define RX_SIZE      256u
#define TX_SIZE      1024u
#define AT_TIMEOUT   400u        /* ms for a reply to start */
#define AT_IDLE      60u         /* ms of silence that ends a reply */
#define NAME_MAX     12

static volatile uint16_t rx_head, rx_tail;
static uint8_t rx_buf[RX_SIZE];
static volatile uint16_t tx_head, tx_tail;
static uint8_t tx_buf[TX_SIZE];

/* Probe results, readable with the debugger (STM32_Programmer_CLI -r8) */
#define PROBES 18
volatile struct {
  uint32_t rx_total;               /* bytes received since boot */
  uint8_t  bytes[PROBES];          /* reply length per probe */
  char     reply[32];              /* first non-empty reply */
  uint32_t reply_baud;
  uint8_t  reply_crlf;
} ble_diag;

static BleState state = BLE_ABSENT;
static uint32_t baud;
static char name[NAME_MAX + 1];
static char pin[7];

/* ------------------------------------------------------------ UART7 interrupt */

void APP_UART7_IRQ(void)
{
  uint32_t sr = UART7->SR;
  if (sr & (USART_SR_RXNE | USART_SR_ORE))
  {
    uint8_t b = (uint8_t)UART7->DR;          /* SR then DR also clears an overrun */
    ble_diag.rx_total++;
    uint16_t next = (uint16_t)((rx_head + 1u) % RX_SIZE);
    if ((sr & USART_SR_RXNE) && next != rx_tail)
    {
      rx_buf[rx_head] = b;
      rx_head = next;
    }
  }
  if ((UART7->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE))
  {
    if (tx_tail == tx_head)
    {
      UART7->CR1 &= ~USART_CR1_TXEIE;        /* all sent */
    }
    else
    {
      UART7->DR = tx_buf[tx_tail];
      tx_tail = (uint16_t)((tx_tail + 1u) % TX_SIZE);
    }
  }
}

static int P_RxPop(uint8_t *b)
{
  if (rx_tail == rx_head) return 0;
  *b = rx_buf[rx_tail];
  rx_tail = (uint16_t)((rx_tail + 1u) % RX_SIZE);
  return 1;
}

/* queue bytes for sending; waits (App task) while the buffer is full */
static void P_Send(const char *s, size_t len)
{
  for (size_t i = 0; i < len; i++)
  {
    uint16_t next = (uint16_t)((tx_head + 1u) % TX_SIZE);
    for (uint32_t t = 0; next == tx_tail; t++)
    {
      if (t > 2000u) return;                 /* stuck: drop the rest */
      osDelay(1);
    }
    tx_buf[tx_head] = (uint8_t)s[i];
    tx_head = next;
    UART7->CR1 |= USART_CR1_TXEIE;
  }
}

static void P_WaitSent(void)
{
  for (uint32_t t = 0; (tx_tail != tx_head || !(UART7->SR & USART_SR_TC)) && t < 500u; t++)
  {
    osDelay(1);
  }
}

/* console output: only while someone is connected (else it would be AT input) */
static void P_ConsoleWrite(const char *s, size_t len)
{
  if (state == BLE_CONNECTED) P_Send(s, len);
}

/* ------------------------------------------------------------ AT commands */

static void P_SetBaud(uint32_t b)
{
  huart7.Init.BaudRate = b;
  HAL_UART_Init(&huart7);
  UART7->CR1 |= USART_CR1_RXNEIE;
  baud = b;
}

/* send an AT command and collect the reply until AT_IDLE ms of silence */
static int P_At(const char *cmd, char *reply, size_t len)
{
  uint8_t b;
  size_t n = 0;
  while (P_RxPop(&b)) { }                    /* drop old bytes */
  P_Send(cmd, strlen(cmd));
  P_WaitSent();
  uint32_t start = osKernelGetTickCount(), last = 0;
  for (;;)
  {
    uint32_t now = osKernelGetTickCount();
    if (P_RxPop(&b))
    {
      if (n + 1u < len) reply[n++] = (char)b;
      last = now;
      continue;
    }
    if (n == 0u && now - start > AT_TIMEOUT) break;
    if (n > 0u && now - last > AT_IDLE) break;
    osDelay(1);
  }
  reply[n] = '\0';
  return (int)n;
}

static int P_Cmd(const char *cmd, char *reply, size_t len);

static int P_AtOk(const char *cmd, const char *expect)
{
  char reply[48];
  P_Cmd(cmd, reply, sizeof reply);
  return strstr(reply, expect) != NULL ? 0 : -1;
}

/* "OK+Get:xxx" / "OK+NAME:xxx" -> xxx */
static void P_Value(const char *reply, char *out, size_t len)
{
  const char *c = strchr(reply, ':');
  out[0] = '\0';
  if (c != NULL)
  {
    snprintf(out, len, "%s", c + 1);
  }
}

static void P_ReadSettings(void)
{
  char reply[48];
  P_Cmd("AT+NAME?", reply, sizeof reply);
  P_Value(reply, name, sizeof name);
  pin[0] = '\0';
  P_Cmd("AT+TYPE?", reply, sizeof reply);
  if (strstr(reply, ":0") == NULL)           /* PIN required: read it */
  {
    P_Cmd("AT+PASS?", reply, sizeof reply);
    P_Value(reply, pin, sizeof pin);
  }
}

static uint8_t crlf;               /* 1 = module wants "AT...\r\n" (clone firmware) */

static int P_Cmd(const char *cmd, char *reply, size_t len)
{
  char line[40];
  snprintf(line, sizeof line, crlf ? "%s\r\n" : "%s", cmd);
  return P_At(line, reply, len);
}

void Ble_Init(void)
{
  static const uint32_t bauds[] = { 9600u, 115200u, 57600u, 38400u, 19200u, 4800u, 2400u, 1200u, 230400u };
  char reply[32];
  int probe = 0;

  Cli_SetWriter(CLI_PORT_BLE, P_ConsoleWrite);
  state = BLE_ABSENT;
  for (size_t i = 0; i < sizeof bauds / sizeof bauds[0] && state == BLE_ABSENT; i++)
  {
    P_SetBaud(bauds[i]);
    for (crlf = 0; crlf < 2u && state == BLE_ABSENT; crlf++)
    {
      int n = P_Cmd("AT", reply, sizeof reply);
      if (probe < PROBES) ble_diag.bytes[probe++] = (uint8_t)n;
      if (n > 0 && ble_diag.reply[0] == '\0')
      {
        memcpy((char *)ble_diag.reply, reply, sizeof reply);
        ble_diag.reply_baud = bauds[i];
        ble_diag.reply_crlf = crlf;
      }
      if (strstr(reply, "OK") != NULL) { state = BLE_IDLE; break; }
    }
  }
  if (state == BLE_ABSENT)
  {
    crlf = 0;
    P_SetBaud(9600u);
    /* no answer: no module, or a phone is connected (then "AT" went to it);
     * data from the phone will still be accepted */
    LOG_W("BLE: HM-11 does not answer (not fitted, or already connected)");
    return;
  }
  P_Cmd("AT+NOTI1", reply, sizeof reply);    /* "OK+CONN" / "OK+LOST" notifications */
  P_ReadSettings();
  if (strcmp(name, "HMSoft") == 0 || name[0] == '\0')
  {
    Ble_SetName("ScoreBoard");
  }
  LOG_I("BLE: HM-11 \"%s\" at %lu baud%s%s", name, (unsigned long)baud,
        crlf ? " (CR LF commands)" : "", pin[0] ? ", PIN set" : "");
}

int Ble_SetName(const char *n)
{
  char cmd[32];
  size_t len = strlen(n);
  if (state != BLE_IDLE || len == 0u || len > NAME_MAX) return -1;
  for (size_t i = 0; i < len; i++)
  {
    char c = n[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'))
      return -1;
  }
  snprintf(cmd, sizeof cmd, "AT+NAME%s", n);
  if (P_AtOk(cmd, "OK") != 0) return -1;
  P_AtOk("AT+RESET", "OK");                  /* the new name is used after a module reset */
  osDelay(800);
  snprintf(name, sizeof name, "%s", n);
  return 0;
}

int Ble_SetPin(const char *p)
{
  char cmd[24];
  if (state != BLE_IDLE) return -1;
  if (p == NULL)
  {
    if (P_AtOk("AT+TYPE0", "OK") != 0) return -1;
    pin[0] = '\0';
  }
  else
  {
    if (strlen(p) != 6u || strspn(p, "0123456789") != 6u) return -1;
    snprintf(cmd, sizeof cmd, "AT+PASS%s", p);
    if (P_AtOk(cmd, "OK") != 0 || P_AtOk("AT+TYPE2", "OK") != 0) return -1;   /* 2 = PIN + bond */
    snprintf(pin, sizeof pin, "%s", p);
  }
  P_AtOk("AT+RESET", "OK");
  osDelay(800);
  return 0;
}

/* ------------------------------------------------------------ data from the module */

/* "OK+CONN" / "OK+LOST" in the byte stream are module notifications */
static const char *const notes[] = { "OK+CONN", "OK+LOST" };
static char held[8];
static size_t held_len;

static void P_Deliver(char c)
{
  if (state != BLE_CONNECTED)                /* data without "OK+CONN" (no NOTI support) */
  {
    state = BLE_CONNECTED;
  }
  Cli_InputFrom(CLI_PORT_BLE, c);
}

static void P_Byte(char c)
{
  held[held_len++] = c;
  held[held_len] = '\0';
  for (int i = 0; i < 2; i++)
  {
    if (strncmp(notes[i], held, held_len) == 0)
    {
      if (held_len == strlen(notes[i]))      /* complete notification */
      {
        held_len = 0;
        if (i == 0)
        {
          state = BLE_CONNECTED;
          LOG_I("BLE: connected");
          Cli_PortConnected(CLI_PORT_BLE);
        }
        else
        {
          state = BLE_IDLE;
          LOG_I("BLE: disconnected");
        }
      }
      return;                                /* (still) a notification prefix: hold it */
    }
  }
  for (size_t i = 0; i < held_len; i++)      /* not a notification: console input */
  {
    P_Deliver(held[i]);
  }
  held_len = 0;
}

void Ble_Poll(void)
{
  uint8_t b;
  while (P_RxPop(&b))
  {
    P_Byte((char)b);
  }
}

BleState Ble_State(void)    { return state; }
const char *Ble_Name(void)  { return name; }
uint32_t Ble_Baud(void)     { return baud; }
const char *Ble_Pin(void)   { return pin; }

const char *Ble_StateName(void)
{
  switch (state)
  {
    case BLE_CONNECTED: return "connected";
    case BLE_IDLE:      return "waiting";
    default:            return "no module";
  }
}

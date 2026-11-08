/*
 * i2c_selftest.c - one-shot I2C / EEPROM hardware test, printed on USART3.
 *
 * 1. Line check: reads PB6, PB8 and PB9 with the internal pull-DOWN enabled.
 *    A line that still reads 1 has an external pull-up resistor.
 * 2. For SCL = PB6 and SCL = PB8 (SDA = PB9 in both cases, internal pull-ups on):
 *    - frees a stuck bus (9 clock pulses),
 *    - scans all 7-bit addresses,
 *    - if the EEPROM (0xA0) answers: reads the first 16 bytes and the first
 *      player name (address 32), then writes 0x5A to TEST_ADDR, reads it back
 *      and restores the original value.
 * 3. Restores I2C1 to the .ioc configuration and PB8 to the OLED reset output.
 *
 * Must run before TFT_INIT(): while SCL = PB8 is tested, the OLED reset line
 * (also PB8) toggles.
 */
#include "i2c_selftest.h"
#include "app_hal.h"
#include "app_main.h"
#include "Ez_USART.h"
#include "i2c1_bus.h"
#include <stdio.h>

#define EE_ADDR       0xA0u
#define TEST_ADDR     0x0FF0u     /* spare byte, inside a 24LC32 (4 KB) */
#define T_MS          20u

#if APP_I2C_SELFTEST
static char line[96];

static void P_Print(const char *s)
{
  USART_puts(USART3, (char *)s);
}

static void P_PinMode(uint16_t pin, uint32_t mode, uint32_t pull)
{
  GPIO_InitTypeDef g = {0};
  g.Pin = pin;
  g.Mode = mode;
  g.Pull = pull;
  g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  g.Alternate = (mode == GPIO_MODE_AF_OD) ? GPIO_AF4_I2C1 : 0u;
  HAL_GPIO_Init(GPIOB, &g);
}

static void P_LineCheck(void)
{
  const uint16_t pins[3] = { GPIO_PIN_6, GPIO_PIN_8, GPIO_PIN_9 };
  const char *names[3] = { "PB6", "PB8", "PB9" };

  P_Print("\r\n[I2C TEST] line check (internal pull-down on, 1 = external pull-up present)\r\n");
  for (int i = 0; i < 3; i++)
  {
    P_PinMode(pins[i], GPIO_MODE_INPUT, GPIO_PULLDOWN);
  }
  HAL_Delay(2);
  for (int i = 0; i < 3; i++)
  {
    snprintf(line, sizeof line, "  %s = %d\r\n", names[i],
             HAL_GPIO_ReadPin(GPIOB, pins[i]) == GPIO_PIN_SET ? 1 : 0);
    P_Print(line);
  }
}

/* Clock out up to 9 pulses on SCL so a slave stuck mid-byte releases SDA. */
static void P_BusRecover(uint16_t scl)
{
  P_PinMode(GPIO_PIN_9, GPIO_MODE_INPUT, GPIO_PULLUP);
  P_PinMode(scl, GPIO_MODE_OUTPUT_OD, GPIO_PULLUP);
  HAL_GPIO_WritePin(GPIOB, scl, GPIO_PIN_SET);
  HAL_Delay(1);
  for (int i = 0; i < 9 && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_RESET; i++)
  {
    HAL_GPIO_WritePin(GPIOB, scl, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, scl, GPIO_PIN_SET);
    HAL_Delay(1);
  }
  snprintf(line, sizeof line, "  idle levels: SCL=%d SDA=%d\r\n",
           HAL_GPIO_ReadPin(GPIOB, scl) == GPIO_PIN_SET,
           HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_SET);
  P_Print(line);
}

/* Route I2C1 to SCL = scl (PB6 or PB8), SDA = PB9, internal pull-ups on. */
static void P_I2cOnPins(uint16_t scl)
{
  HAL_I2C_DeInit(&hi2c1);
  P_BusRecover(scl);
  HAL_I2C_Init(&hi2c1);                       /* MspInit maps PB6/PB9 (from the .ioc) */
  P_PinMode(GPIO_PIN_6 | GPIO_PIN_8, GPIO_MODE_INPUT, GPIO_PULLUP);
  P_PinMode(scl | GPIO_PIN_9, GPIO_MODE_AF_OD, GPIO_PULLUP);
  /* reset the peripheral after the pin change (clears a latched BUSY flag) */
  hi2c1.Instance->CR1 |= I2C_CR1_SWRST;
  hi2c1.Instance->CR1 &= ~I2C_CR1_SWRST;
  HAL_I2C_Init(&hi2c1);
}

static void P_TestOnPins(uint16_t scl, const char *name)
{
  int found = 0;

  snprintf(line, sizeof line, "\r\n[I2C TEST] SCL=%s SDA=PB9\r\n", name);
  P_Print(line);
  P_I2cOnPins(scl);

  P_Print("  scan:");
  for (uint16_t a = 0x08; a < 0x78; a++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(a << 1), 2, 5) == HAL_OK)
    {
      snprintf(line, sizeof line, " 0x%02X(8-bit 0x%02X)", a, a << 1);
      P_Print(line);
      found++;
    }
  }
  snprintf(line, sizeof line, "%s\r\n", found ? "" : " nothing found");
  P_Print(line);
  snprintf(line, sizeof line, "  HAL state=%d error=0x%02lX SR2=0x%04lX\r\n",
           (int)hi2c1.State, (unsigned long)hi2c1.ErrorCode,
           (unsigned long)hi2c1.Instance->SR2);
  P_Print(line);

  if (HAL_I2C_IsDeviceReady(&hi2c1, EE_ADDR, 2, 5) != HAL_OK)
  {
    P_Print("  EEPROM 0xA0: no answer\r\n");
    return;
  }

  /* read: first 16 bytes and the first player name (old map: name at 32) */
  uint8_t buf[32] = {0};
  if (HAL_I2C_Mem_Read(&hi2c1, EE_ADDR, 0x0000, I2C_MEMADD_SIZE_16BIT, buf, 16, T_MS) == HAL_OK)
  {
    P_Print("  0x0000:");
    for (int i = 0; i < 16; i++)
    {
      snprintf(line, sizeof line, " %02X", buf[i]);
      P_Print(line);
    }
    P_Print("\r\n");
  }
  else
  {
    P_Print("  read 0x0000 FAILED\r\n");
  }
  if (HAL_I2C_Mem_Read(&hi2c1, EE_ADDR, 32, I2C_MEMADD_SIZE_16BIT, buf, 31, T_MS) == HAL_OK)
  {
    buf[31] = 0;
    for (int i = 0; i < 31; i++) if (buf[i] && (buf[i] < 0x20 || buf[i] > 0x7E)) buf[i] = '.';
    snprintf(line, sizeof line, "  name@32: \"%s\"\r\n", (char *)buf);
    P_Print(line);
  }

  /* write test with restore */
  uint8_t orig = 0, rd = 0, pattern = 0x5A;
  if (HAL_I2C_Mem_Read(&hi2c1, EE_ADDR, TEST_ADDR, I2C_MEMADD_SIZE_16BIT, &orig, 1, T_MS) != HAL_OK)
  {
    P_Print("  write test: read of test byte FAILED\r\n");
    return;
  }
  if (orig == pattern) pattern = 0xA5;
  HAL_GPIO_WritePin(EE_WP_GPIO_Port, EE_WP_Pin, GPIO_PIN_RESET);   /* WP low = writable */
  HAL_StatusTypeDef w = HAL_I2C_Mem_Write(&hi2c1, EE_ADDR, TEST_ADDR, I2C_MEMADD_SIZE_16BIT, &pattern, 1, T_MS);
  I2C1Bus_WaitReady(EE_ADDR, 10);                                    /* wait for write cycle */
  HAL_I2C_Mem_Read(&hi2c1, EE_ADDR, TEST_ADDR, I2C_MEMADD_SIZE_16BIT, &rd, 1, T_MS);
  HAL_I2C_Mem_Write(&hi2c1, EE_ADDR, TEST_ADDR, I2C_MEMADD_SIZE_16BIT, &orig, 1, T_MS);
  I2C1Bus_WaitReady(EE_ADDR, 10);
  snprintf(line, sizeof line, "  write test @0x%04X: wrote %02X (%s) read %02X -> %s (restored %02X)\r\n",
           TEST_ADDR, pattern, w == HAL_OK ? "ok" : "ERR", rd, rd == pattern ? "PASS" : "FAIL", orig);
  P_Print(line);
}

#endif /* APP_I2C_SELFTEST */

void I2C_SelfTest(void)
{
#if APP_I2C_SELFTEST
  P_LineCheck();
  P_TestOnPins(GPIO_PIN_6, "PB6");
  /* SCL = PB8 was tested on 2026-11-08: no device there, the EEPROM is on PB6. */

  /* back to the .ioc configuration */
  HAL_I2C_DeInit(&hi2c1);
  P_PinMode(GPIO_PIN_8, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);          /* OLED_RES */
  HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_RESET);
  MX_I2C1_Init();
  P_Print("[I2C TEST] done\r\n\r\n");
#endif
}

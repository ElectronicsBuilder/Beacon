/*
 * ee_i2c.c - I2C transfer helpers used by EEPROM.c (was EEPROM/i2c.c).
 *
 * I2C1 pins come from the .ioc (currently SCL=PB6, SDA=PB9, internal pull-ups).
 * The original firmware used SCL=PB8; see i2c_selftest.c to check the board.
 */
#include "ee_i2c.h"

#define EE_I2C_TIMEOUT_MS 20u
#ifndef OK
#define OK    (0)
#endif
#ifndef ERROR_I2C
#define ERROR_I2C (-1)
#endif

static uint16_t P_AddrSize(uint8_t ptrlen)
{
  return (ptrlen >= 2u) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;
}

void I2C_BusInit(void)
{
  /* Configured by CubeMX (MX_I2C1_Init). */
}

int I2C_CheckDevice(uint8_t dev_addr)
{
  return (HAL_I2C_IsDeviceReady(&hi2c1, dev_addr, 1, EE_I2C_TIMEOUT_MS) == HAL_OK) ? OK : ERROR_I2C;
}

int I2C_ReadTransfer(uint8_t dev_addr, uint8_t *buffer, int cnt, uint32_t ptr, uint8_t ptrlen)
{
  HAL_StatusTypeDef st;
  if (buffer == NULL || cnt <= 0 || ptrlen > 4u) return ERROR_I2C;
  if (ptrlen == 0u)
    st = HAL_I2C_Master_Receive(&hi2c1, dev_addr, buffer, (uint16_t)cnt, EE_I2C_TIMEOUT_MS);
  else
    st = HAL_I2C_Mem_Read(&hi2c1, dev_addr, (uint16_t)ptr, P_AddrSize(ptrlen), buffer, (uint16_t)cnt, EE_I2C_TIMEOUT_MS);
  return (st == HAL_OK) ? OK : ERROR_I2C;
}

int I2C_WriteTransfer(uint8_t dev_addr, uint8_t *buffer, int cnt, uint32_t ptr, uint8_t ptrlen)
{
  HAL_StatusTypeDef st;
  if (buffer == NULL || cnt <= 0 || ptrlen > 4u) return ERROR_I2C;
  if (ptrlen == 0u)
    st = HAL_I2C_Master_Transmit(&hi2c1, dev_addr, buffer, (uint16_t)cnt, EE_I2C_TIMEOUT_MS);
  else
    st = HAL_I2C_Mem_Write(&hi2c1, dev_addr, (uint16_t)ptr, P_AddrSize(ptrlen), buffer, (uint16_t)cnt, EE_I2C_TIMEOUT_MS);
  return (st == HAL_OK) ? OK : ERROR_I2C;
}

int I2C_ReadTransfer_single(uint8_t dev_addr, uint8_t data, int cnt, uint32_t ptr, uint8_t ptrlen)
{
  /* As in the original, the byte is read into a by-value argument (result unused). */
  (void)cnt;
  return I2C_ReadTransfer(dev_addr, &data, 1, ptr, ptrlen);
}

int I2C_WriteTransfer_single(uint8_t dev_addr, uint8_t data, int cnt, uint32_t ptr, uint8_t ptrlen)
{
  (void)cnt;
  return I2C_WriteTransfer(dev_addr, &data, 1, ptr, ptrlen);
}

int I2C_write2(I2C_TypeDef *I2Cx, uint8_t data)
{
  (void)I2Cx;
  return (HAL_I2C_Master_Transmit(&hi2c1, 0xA0, &data, 1, EE_I2C_TIMEOUT_MS) == HAL_OK) ? OK : ERROR_I2C;
}

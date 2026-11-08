//--------------------------------------------------------------
// File     : i2c1_bus.c  (I2C1 for the 24LCxx EEPROM, HAL version)
// Same API as the original UB driver; slave addresses are 8-bit (0xA0).
//--------------------------------------------------------------
#include "i2c1_bus.h"
#include "delay.h"
#include "cmsis_os.h"

/* Global variables declared in i2c1_bus.h (defined here once) */


#define I2C1_TIMEOUT_MS 10u







int16_t I2C1Bus_ReadByte16(uint8_t slave_adr, uint16_t adr)
{
  uint8_t wert;
  if (HAL_I2C_Mem_Read(&hi2c1, slave_adr, adr, I2C_MEMADD_SIZE_16BIT, &wert, 1, I2C1_TIMEOUT_MS) != HAL_OK)
    return -1;
  return wert;
}

int16_t I2C1Bus_WriteByte16(uint8_t slave_adr, uint16_t adr, uint8_t wert)
{
  if (HAL_I2C_Mem_Write(&hi2c1, slave_adr, adr, I2C_MEMADD_SIZE_16BIT, &wert, 1, I2C1_TIMEOUT_MS) != HAL_OK)
    return -1;
  return 0;
}


/* 100 kHz: about 0.1 ms per byte, plus the base timeout */
static uint32_t P_Timeout(uint16_t len)
{
  return I2C1_TIMEOUT_MS + len / 4u;
}

int16_t I2C1Bus_Read16(uint8_t slave_adr, uint16_t adr, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(&hi2c1, slave_adr, adr, I2C_MEMADD_SIZE_16BIT, buf, len, P_Timeout(len)) == HAL_OK ? 0 : -1;
}

int16_t I2C1Bus_Write16(uint8_t slave_adr, uint16_t adr, const uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Write(&hi2c1, slave_adr, adr, I2C_MEMADD_SIZE_16BIT, (uint8_t *)buf, len,
                           P_Timeout(len)) == HAL_OK ? 0 : -1;
}

int16_t I2C1Bus_WaitReady(uint8_t slave_adr, uint32_t timeout_ms)
{
  /* Poll once per millisecond; a 24LCxx write cycle takes up to 5 ms.
   * osDelay lets the USB task run meanwhile. */
  for (uint32_t t = 0; t <= timeout_ms; t++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, slave_adr, 1, 2) == HAL_OK)
      return 0;
    osDelay(1);
  }
  return -1;
}

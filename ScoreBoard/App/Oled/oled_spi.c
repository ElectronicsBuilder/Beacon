//--------------------------------------------------------------
// File     : oled_spi.c  (SPI1 for the SSD1351 OLED, HAL version)
// The peripheral is configured by CubeMX (mode 3, prescaler 4).
// Byte transfers go straight to the data register, as in the original
// driver, because the display code sends one byte per call.
//--------------------------------------------------------------
#include "oled_spi.h"

ErrorStatus OledSpi_Init(SPI1_Mode_t mode)
{
  if (mode != SPI_MODE_3)
  {
    return ERROR; /* the board only uses mode 3 (set in the .ioc) */
  }
  __HAL_SPI_ENABLE(&hspi1);
  return SUCCESS;
}

uint8_t OledSpi_SendByte(uint8_t wert)
{
  SPI_TypeDef *spi = hspi1.Instance;

  while ((spi->SR & SPI_SR_TXE) == 0u) { }
  *(__IO uint8_t *)&spi->DR = wert;
  while ((spi->SR & SPI_SR_RXNE) == 0u) { }
  return (uint8_t)spi->DR;
}

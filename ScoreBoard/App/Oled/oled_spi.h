//--------------------------------------------------------------
// File     : oled_spi.h  (SPI1 for the SSD1351 OLED, HAL version)
// SCK=PA5, MOSI=PB5, mode 3, 21 MHz - configured by CubeMX (MX_SPI1_Init)
//--------------------------------------------------------------
#ifndef __STM32F4_OledSpi_H
#define __STM32F4_OledSpi_H

#include "app_hal.h"

typedef enum {
  SPI_MODE_0 = 0,  // CPOL=0, CPHA=0
  SPI_MODE_1,      // CPOL=0, CPHA=1
  SPI_MODE_2,      // CPOL=1, CPHA=0
  SPI_MODE_3       // CPOL=1, CPHA=1
} SPI1_Mode_t;

ErrorStatus OledSpi_Init(SPI1_Mode_t mode);
uint8_t OledSpi_SendByte(uint8_t wert);

#endif // __STM32F4_OledSpi_H

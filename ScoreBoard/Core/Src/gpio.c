/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED_GREEN_Pin|LED_ORANGE_Pin|LED_RED_Pin|LED_BLUE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, MTX_B_Pin|MTX_C_Pin|MTX_D_Pin|MTX_PC2_NC_Pin
                          |MTX_B2_Pin|MTX_G2_Pin|MTX_R2_Pin|MTX_B1_Pin
                          |MTX_G1_Pin|MTX_R1_Pin|MTX_LAT_Pin|MTX_OE_Pin
                          |MTX_A_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RADIO_CONFIG_GPIO_Port, RADIO_CONFIG_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RADIO_EN_GPIO_Port, RADIO_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, EE_WP_Pin|OLED_RES_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_GREEN_Pin LED_ORANGE_Pin LED_RED_Pin LED_BLUE_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin|LED_ORANGE_Pin|LED_RED_Pin|LED_BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : MTX_B_Pin MTX_C_Pin MTX_D_Pin MTX_PC2_NC_Pin
                           MTX_B2_Pin MTX_G2_Pin MTX_R2_Pin MTX_B1_Pin
                           MTX_G1_Pin MTX_R1_Pin MTX_LAT_Pin MTX_OE_Pin
                           MTX_A_Pin */
  GPIO_InitStruct.Pin = MTX_B_Pin|MTX_C_Pin|MTX_D_Pin|MTX_PC2_NC_Pin
                          |MTX_B2_Pin|MTX_G2_Pin|MTX_R2_Pin|MTX_B1_Pin
                          |MTX_G1_Pin|MTX_R1_Pin|MTX_LAT_Pin|MTX_OE_Pin
                          |MTX_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PWR_EN_Pin */
  GPIO_InitStruct.Pin = USB_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PWR_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RADIO_CONFIG_Pin RADIO_EN_Pin */
  GPIO_InitStruct.Pin = RADIO_CONFIG_Pin|RADIO_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : REG_EN_Pin BTN_SW3_Pin BTN_SW4_Pin OTG_HS_ID_Pin
                           OTG_HS_VBUS_Pin OTG_FS_OC_Pin */
  GPIO_InitStruct.Pin = REG_EN_Pin|BTN_SW3_Pin|BTN_SW4_Pin|OTG_HS_ID_Pin
                          |OTG_HS_VBUS_Pin|OTG_FS_OC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : BLE_TXD_Pin BLE_RXD_Pin FLASH_WP_Pin FLASH_HOLD_Pin
                           SPI4_CS_Pin SPI4_SCK_Pin SPI4_MISO_Pin SPI4_MOSI_Pin
                           FLASH_CE_Pin WIFI_TXD_Pin WIFI_RXD_Pin */
  GPIO_InitStruct.Pin = BLE_TXD_Pin|BLE_RXD_Pin|FLASH_WP_Pin|FLASH_HOLD_Pin
                          |SPI4_CS_Pin|SPI4_SCK_Pin|SPI4_MISO_Pin|SPI4_MOSI_Pin
                          |FLASH_CE_Pin|WIFI_TXD_Pin|WIFI_RXD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : OLED_CS_Pin OLED_DC_Pin */
  GPIO_InitStruct.Pin = OLED_CS_Pin|OLED_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : WIFI_RESET_Pin */
  GPIO_InitStruct.Pin = WIFI_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(WIFI_RESET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : EE_WP_Pin OLED_RES_Pin */
  GPIO_InitStruct.Pin = EE_WP_Pin|OLED_RES_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

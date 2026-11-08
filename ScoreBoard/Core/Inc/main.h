/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_GREEN_Pin GPIO_PIN_2
#define LED_GREEN_GPIO_Port GPIOE
#define LED_ORANGE_Pin GPIO_PIN_3
#define LED_ORANGE_GPIO_Port GPIOE
#define LED_RED_Pin GPIO_PIN_4
#define LED_RED_GPIO_Port GPIOE
#define LED_BLUE_Pin GPIO_PIN_5
#define LED_BLUE_GPIO_Port GPIOE
#define MTX_B_Pin GPIO_PIN_13
#define MTX_B_GPIO_Port GPIOC
#define MTX_C_Pin GPIO_PIN_14
#define MTX_C_GPIO_Port GPIOC
#define MTX_D_Pin GPIO_PIN_15
#define MTX_D_GPIO_Port GPIOC
#define USB_PWR_EN_Pin GPIO_PIN_0
#define USB_PWR_EN_GPIO_Port GPIOC
#define MTX_E_Pin GPIO_PIN_2
#define MTX_E_GPIO_Port GPIOC
#define MTX_B2_Pin GPIO_PIN_3
#define MTX_B2_GPIO_Port GPIOC
#define RADIO_CONFIG_Pin GPIO_PIN_1
#define RADIO_CONFIG_GPIO_Port GPIOA
#define RADIO_TXD_Pin GPIO_PIN_2
#define RADIO_TXD_GPIO_Port GPIOA
#define RADIO_RXD_Pin GPIO_PIN_3
#define RADIO_RXD_GPIO_Port GPIOA
#define OLED_SCK_Pin GPIO_PIN_5
#define OLED_SCK_GPIO_Port GPIOA
#define RADIO_EN_Pin GPIO_PIN_7
#define RADIO_EN_GPIO_Port GPIOA
#define MTX_G2_Pin GPIO_PIN_4
#define MTX_G2_GPIO_Port GPIOC
#define MTX_R2_Pin GPIO_PIN_5
#define MTX_R2_GPIO_Port GPIOC
#define REG_EN_Pin GPIO_PIN_0
#define REG_EN_GPIO_Port GPIOB
#define BTN_SW3_Pin GPIO_PIN_1
#define BTN_SW3_GPIO_Port GPIOB
#define BTN_SW4_Pin GPIO_PIN_2
#define BTN_SW4_GPIO_Port GPIOB
#define BLE_TXD_Pin GPIO_PIN_7
#define BLE_TXD_GPIO_Port GPIOE
#define BLE_RXD_Pin GPIO_PIN_8
#define BLE_RXD_GPIO_Port GPIOE
#define FLASH_WP_Pin GPIO_PIN_9
#define FLASH_WP_GPIO_Port GPIOE
#define FLASH_HOLD_Pin GPIO_PIN_10
#define FLASH_HOLD_GPIO_Port GPIOE
#define SPI4_CS_Pin GPIO_PIN_11
#define SPI4_CS_GPIO_Port GPIOE
#define SPI4_SCK_Pin GPIO_PIN_12
#define SPI4_SCK_GPIO_Port GPIOE
#define SPI4_MISO_Pin GPIO_PIN_13
#define SPI4_MISO_GPIO_Port GPIOE
#define SPI4_MOSI_Pin GPIO_PIN_14
#define SPI4_MOSI_GPIO_Port GPIOE
#define FLASH_CE_Pin GPIO_PIN_15
#define FLASH_CE_GPIO_Port GPIOE
#define USART3_TX_Pin GPIO_PIN_10
#define USART3_TX_GPIO_Port GPIOB
#define USART3_RX_Pin GPIO_PIN_11
#define USART3_RX_GPIO_Port GPIOB
#define OTG_HS_ID_Pin GPIO_PIN_12
#define OTG_HS_ID_GPIO_Port GPIOB
#define OTG_HS_VBUS_Pin GPIO_PIN_13
#define OTG_HS_VBUS_GPIO_Port GPIOB
#define OLED_CS_Pin GPIO_PIN_9
#define OLED_CS_GPIO_Port GPIOD
#define OLED_DC_Pin GPIO_PIN_10
#define OLED_DC_GPIO_Port GPIOD
#define MTX_B1_Pin GPIO_PIN_6
#define MTX_B1_GPIO_Port GPIOC
#define MTX_G1_Pin GPIO_PIN_7
#define MTX_G1_GPIO_Port GPIOC
#define MTX_R1_Pin GPIO_PIN_8
#define MTX_R1_GPIO_Port GPIOC
#define MTX_CLK_Pin GPIO_PIN_9
#define MTX_CLK_GPIO_Port GPIOC
#define WIFI_RESET_Pin GPIO_PIN_8
#define WIFI_RESET_GPIO_Port GPIOA
#define MTX_LAT_Pin GPIO_PIN_10
#define MTX_LAT_GPIO_Port GPIOC
#define MTX_OE_Pin GPIO_PIN_11
#define MTX_OE_GPIO_Port GPIOC
#define MTX_A_Pin GPIO_PIN_12
#define MTX_A_GPIO_Port GPIOC
#define OTG_FS_OC_Pin GPIO_PIN_4
#define OTG_FS_OC_GPIO_Port GPIOB
#define OLED_MOSI_Pin GPIO_PIN_5
#define OLED_MOSI_GPIO_Port GPIOB
#define EE_SCL_Pin GPIO_PIN_6
#define EE_SCL_GPIO_Port GPIOB
#define EE_WP_Pin GPIO_PIN_7
#define EE_WP_GPIO_Port GPIOB
#define OLED_RES_Pin GPIO_PIN_8
#define OLED_RES_GPIO_Port GPIOB
#define EE_SDA_Pin GPIO_PIN_9
#define EE_SDA_GPIO_Port GPIOB
#define WIFI_TXD_Pin GPIO_PIN_0
#define WIFI_TXD_GPIO_Port GPIOE
#define WIFI_RXD_Pin GPIO_PIN_1
#define WIFI_RXD_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

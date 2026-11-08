//--------------------------------------------------------------
// File     : status_led.c  (status LEDs, HAL version)
// The pins are configured by CubeMX (MX_GPIO_Init); LED on = pin high.
//--------------------------------------------------------------
#include "status_led.h"

static const uint16_t LED_PIN[LED_ANZ] = {
  LED_GREEN_Pin, LED_ORANGE_Pin, LED_RED_Pin, LED_BLUE_Pin
};
static GPIO_TypeDef *const LED_PORT[LED_ANZ] = {
  LED_GREEN_GPIO_Port, LED_ORANGE_GPIO_Port, LED_RED_GPIO_Port, LED_BLUE_GPIO_Port
};

void Led_Init(void)
{
  for (int n = 0; n < LED_ANZ; n++)
  {
    Led_Off((LED_NAME_t)n);
  }
}

void Led_Off(LED_NAME_t led_name)
{
  HAL_GPIO_WritePin(LED_PORT[led_name], LED_PIN[led_name], GPIO_PIN_RESET);
}

void Led_On(LED_NAME_t led_name)
{
  HAL_GPIO_WritePin(LED_PORT[led_name], LED_PIN[led_name], GPIO_PIN_SET);
}

void Led_Toggle(LED_NAME_t led_name)
{
  HAL_GPIO_TogglePin(LED_PORT[led_name], LED_PIN[led_name]);
}

void Led_Switch(LED_NAME_t led_name, LED_STATUS_t wert)
{
  if (wert == LED_OFF) Led_Off(led_name);
  else                 Led_On(led_name);
}

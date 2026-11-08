//--------------------------------------------------------------
// File     : dig_out.c  (digital outputs, HAL version)
// Pins and their reset levels are set by CubeMX (MX_GPIO_Init).
// USB_ON and LED_POWER are not driven: the old firmware never drove them
// (PC0 is handled by the USB host stack, PB0 is left as an input).
//--------------------------------------------------------------
#include "dig_out.h"

static int P_DigOut_Lookup(DOUT_NAME_t dig_pin, GPIO_TypeDef **port, uint16_t *pin)
{
  switch (dig_pin)
  {
    case RADIO_EN:     *port = RADIO_EN_GPIO_Port;     *pin = RADIO_EN_Pin;     return 1;
    case RADIO_CONFIG: *port = RADIO_CONFIG_GPIO_Port; *pin = RADIO_CONFIG_Pin; return 1;
    case EE_WP:        *port = EE_WP_GPIO_Port;        *pin = EE_WP_Pin;        return 1;
    default:           return 0;
  }
}

void DigOut_Init(void)
{
  /* Same start-up levels as the original table. */
  DigOut_Lo(RADIO_EN);
  DigOut_Lo(RADIO_CONFIG);
  DigOut_Lo(EE_WP);
}

void DigOut_Pin(DOUT_NAME_t dig_pin, BitAction wert)
{
  GPIO_TypeDef *port;
  uint16_t pin;
  if (P_DigOut_Lookup(dig_pin, &port, &pin))
  {
    HAL_GPIO_WritePin(port, pin, (wert == Bit_RESET) ? GPIO_PIN_RESET : GPIO_PIN_SET);
  }
}

void DigOut_Lo(DOUT_NAME_t dig_pin)
{
  DigOut_Pin(dig_pin, Bit_RESET);
}

void DigOut_Hi(DOUT_NAME_t dig_pin)
{
  DigOut_Pin(dig_pin, Bit_SET);
}

void DigOut_Toggle(DOUT_NAME_t dig_pin)
{
  GPIO_TypeDef *port;
  uint16_t pin;
  if (P_DigOut_Lookup(dig_pin, &port, &pin))
  {
    HAL_GPIO_TogglePin(port, pin);
  }
}

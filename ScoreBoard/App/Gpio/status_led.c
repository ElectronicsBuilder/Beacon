//--------------------------------------------------------------
// File     : status_led.c  (status LEDs PE2..PE5). See status_led.h.
// The pins are configured by CubeMX (MX_GPIO_Init); LED on = pin high.
// TIM7 (not used by CubeMX) is set up here for the software PWM.
//--------------------------------------------------------------
#include "status_led.h"
#include "hm11.h"
#include "EEPROM.h"
#include "rtc_trim.h"
#include "clock.h"

#define PWM_LEVELS   128u                /* duty steps per period */
#define PWM_HZ       32000u              /* interrupt rate: 32 kHz / 128 = 250 Hz */
#define LED_MASK     (LED_GREEN_Pin | LED_ORANGE_Pin | LED_RED_Pin | LED_BLUE_Pin)

static const uint16_t LED_PIN[LED_ANZ] = {
  LED_GREEN_Pin, LED_ORANGE_Pin, LED_RED_Pin, LED_BLUE_Pin
};

static volatile uint8_t duty[LED_ANZ];   /* 0..PWM_LEVELS, read by the interrupt */
static uint8_t brightness = LED_BRIGHTNESS_DEFAULT;
static uint16_t flash_ms[LED_ANZ];
static uint32_t ms;                      /* Led_Poll calls */
static uint32_t test_until;              /* Led_Test: ms value when it ends, 0 = off */

/* ------------------------------------------------------------ PWM (TIM7) */

void TIM7_IRQHandler(void)
{
  static uint8_t phase;
  TIM7->SR = 0;
  uint32_t set = 0;
  for (int n = 0; n < LED_ANZ; n++)
  {
    if (phase < duty[n]) set |= LED_PIN[n];
  }
  GPIOE->BSRR = set | ((uint32_t)(LED_MASK & ~set) << 16);   /* one write, PE2..PE5 only */
  if (++phase >= PWM_LEVELS) phase = 0;
}

static void P_TimerStart(void)
{
  __HAL_RCC_TIM7_CLK_ENABLE();
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  uint32_t clk = ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1) ? pclk1 : 2u * pclk1;
  TIM7->CR1 = 0;
  TIM7->PSC = clk / 1000000u - 1u;       /* 1 MHz */
  TIM7->ARR = 1000000u / PWM_HZ - 1u;
  TIM7->EGR = TIM_EGR_UG;
  TIM7->SR = 0;
  TIM7->DIER = TIM_DIER_UIE;
  HAL_NVIC_SetPriority(TIM7_IRQn, 14, 0);   /* lowest: never in the way of the matrix */
  HAL_NVIC_EnableIRQ(TIM7_IRQn);
  TIM7->CR1 = TIM_CR1_CEN;
}

/* pattern level 0..255 -> duty, scaled linearly by the brightness (LEDs at
 * low current look fairly linear; gamma would squash the patterns at 15 %) */
static uint8_t P_Duty(uint32_t level)
{
  uint32_t d = (level * brightness * PWM_LEVELS + 255u * 100u / 2u) / (255u * 100u);
  if (level > 0u && brightness > 0u && d == 0u) d = 1u;
  return (uint8_t)d;
}

/* ------------------------------------------------------------ public */

void Led_Init(void)
{
  for (int n = 0; n < LED_ANZ; n++) duty[n] = 0;
  HAL_GPIO_WritePin(GPIOE, LED_MASK, GPIO_PIN_RESET);
  P_TimerStart();
}

void Led_SetBrightness(uint8_t percent) { brightness = percent > 100u ? 100u : percent; }
uint8_t Led_GetBrightness(void)         { return brightness; }

void Led_Flash(LED_NAME_t led, uint16_t length_ms)
{
  if (led < LED_ANZ && flash_ms[led] < length_ms) flash_ms[led] = length_ms;
}

void Led_Test(void) { test_until = ms + 4u * 1000u + 1u; }

uint8_t Led_Errors(void)
{
  uint8_t e = 0;
  RtcTrim t;
  if (!eeprom_ok) e |= 1u;
  if (Ble_State() == BLE_ABSENT) e |= 2u;
  if (RtcTrim_Enabled() && RtcTrim_Get(&t) != 0) e |= 4u;
  return e;
}

void Led_Crash(void)
{
  TIM7->CR1 = 0;
  GPIOE->BSRR = LED_RED_Pin | ((uint32_t)(LED_GREEN_Pin | LED_ORANGE_Pin | LED_BLUE_Pin) << 16);
}

/* red blink code: the lowest error bit set blinks n times, then a pause.
 * Cycles through all active errors. */
static uint32_t P_RedLevel(uint8_t errors)
{
  static uint8_t shown = 0;              /* error code being blinked, 1..3 */
  static uint32_t start;
  if (errors == 0u) { shown = 0; return 0; }
  uint32_t t = ms - start;
  uint32_t length = (uint32_t)shown * 400u + 1600u;  /* n x (200 on, 200 off), 1.6 s pause */
  if (shown == 0u || t >= length || !(errors & (1u << (shown - 1u))))
  {
    uint8_t next = shown;
    for (int i = 0; i < 3; i++)                      /* next active error */
    {
      next = (uint8_t)(next % 3u + 1u);
      if (errors & (1u << (next - 1u))) break;
    }
    shown = next;
    start = ms;
    t = 0;
  }
  return (t < (uint32_t)shown * 400u && (t % 400u) < 200u) ? 255u : 0u;
}

void Led_Poll(void)
{
  ms++;
  uint32_t level[LED_ANZ];

  if (test_until != 0u && (int32_t)(test_until - ms) > 0)
  {
    uint32_t which = (4u * 1000u - (test_until - ms)) / 1000u;   /* 0..3, one second each */
    for (int n = 0; n < LED_ANZ; n++) duty[n] = (n == (int)which) ? P_Duty(255u) : 0u;
    return;
  }
  test_until = 0;

  /* green: breathing, 3 s period, never fully off */
  uint32_t p = ms % 3000u;
  uint32_t tri = p < 1500u ? p : 3000u - p;              /* 0..1500 */
  level[LED_GREEN] = 40u + tri * 215u / 1500u;

  /* blue: Bluetooth state */
  switch (Ble_State())
  {
    case BLE_CONNECTED: level[LED_BLUE] = 255u; break;
    case BLE_IDLE:      level[LED_BLUE] = 70u; break;
    default:            level[LED_BLUE] = 0u; break;
  }

  /* orange: clock needs syncing = 1 Hz blink */
  level[LED_ORANGE] = (Clock_IsStale() && (ms % 1000u) < 150u) ? 255u : 0u;

  /* red: error blink codes */
  level[LED_RED] = P_RedLevel(Led_Errors());

  for (int n = 0; n < LED_ANZ; n++)
  {
    if (flash_ms[n] > 0u)                                /* activity: blink against the base */
    {
      flash_ms[n]--;
      level[n] = (level[n] > 128u) ? 20u : 255u;
    }
    duty[n] = P_Duty(level[n]);
  }
}

/* ------------------------------------------------------------ direct control */

void Led_Off(LED_NAME_t led_name)
{
  if (led_name < LED_ANZ) duty[led_name] = 0;
}

void Led_On(LED_NAME_t led_name)
{
  if (led_name < LED_ANZ) duty[led_name] = P_Duty(255u);
}

void Led_Toggle(LED_NAME_t led_name)
{
  if (led_name < LED_ANZ)
  {
    if (duty[led_name]) Led_Off(led_name); else Led_On(led_name);
  }
}

void Led_Switch(LED_NAME_t led_name, LED_STATUS_t wert)
{
  if (wert == LED_OFF) Led_Off(led_name);
  else                 Led_On(led_name);
}

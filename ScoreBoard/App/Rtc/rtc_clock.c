//--------------------------------------------------------------
// File     : rtc_clock.c
// Game clock based on the internal RTC (HAL version).
//
// The RTC runs from the LSI (PC14/PC15 are used by the LED matrix, so there
// is no 32 kHz crystal). Prescalers 127/255 give a 1 Hz calendar tick.
// The time is shown in BCD in SEC_H / MIN_H / HOUR_H, as before.
//--------------------------------------------------------------
#include "rtc_clock.h"
#include "app_main.h"
#include "Ez_USART.h"

static void P_RTC_SetTimeDate(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t format)
{
  RTC_TimeTypeDef t = {0};
  RTC_DateTypeDef d = {0};

  t.Hours = hours;
  t.Minutes = minutes;
  t.Seconds = seconds;
  t.TimeFormat = RTC_HOURFORMAT12_AM;
  t.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  t.StoreOperation = RTC_STOREOPERATION_RESET;
  HAL_RTC_SetTime(&hrtc, &t, format);

  /* Same fixed date as the original firmware: Tuesday 14 July 2015 */
  d.WeekDay = RTC_WEEKDAY_TUESDAY;
  d.Month = RTC_MONTH_JULY;
  d.Date = 0x14;
  d.Year = 0x15;
  HAL_RTC_SetDate(&hrtc, &d, RTC_FORMAT_BCD);
}

static void P_RTC_ReportClock(void)
{
  uint32_t bdcr = RCC->BDCR;

  USART_puts(USART3, (bdcr & RCC_BDCR_LSEON) ? "LSE On\n" : "LSE Off\n");
  USART_puts(USART3, (bdcr & RCC_BDCR_LSERDY) ? "LSE Ready\n" : "LSE Not Ready\n");
  USART_puts(USART3, (bdcr & RCC_BDCR_LSEBYP) ? "LSE ByPass On\n" : "LSE ByPass Off\n");
  USART_puts(USART3, (bdcr & RCC_BDCR_RTCEN) ? "RTC Clock Enabled\n" : "RTC Clock Disabled\n");
  switch (bdcr & RCC_BDCR_RTCSEL)
  {
    case RCC_BDCR_RTCSEL_0: USART_puts(USART3, "RTC Clock Source LSE"); break;
    case RCC_BDCR_RTCSEL_1: USART_puts(USART3, "RTC Clock Source LSI"); break;
    case RCC_BDCR_RTCSEL:   USART_puts(USART3, "RTC Clock Source HSE"); break;
    default:                USART_puts(USART3, "RTC Clock Unknown");    break;
  }
}

// Reset the clock to 00:00:00 and start the 1 s refresh timer (TIM2).
void RTC_RESET_TIME(void)
{
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, FIRST_DATA);
  P_RTC_SetTimeDate(0x00, 0x00, 0x00, RTC_FORMAT_BCD);
  P_RTC_ReportClock();
  INIT_TIMER_CONFIG(RTC_TIMER_PERIPH, RTC_TIMER_IRQ, RTC_TIM_TYPE, RTC_UPDATE_TIME);
}

// Start-up configuration. The original also always started from 00:00:00.
void RTC_Configuration(void)
{
  RTC_RESET_TIME();
}

// Read the current time into SEC_H / MIN_H / HOUR_H (BCD).
void RTC_TimeShow(void)
{
  RTC_TimeTypeDef t;
  RTC_DateTypeDef d;

  HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BCD);
  HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BCD);   /* must follow GetTime to unlock the shadow registers */

  SEC_H = t.Seconds;
  MIN_H = t.Minutes;
  HOUR_H = t.Hours;

#ifdef DEBUG_RTC_CLOCK
  eol(USART3);
  USART_puts(USART3, " DATE -> ");    DISPLAY_INT2(USART3, d.Date, 16, 0);
  USART_puts(USART3, " MONTH -> ");   DISPLAY_INT2(USART3, d.Month, 16, 0);
  USART_puts(USART3, " YEAR -> ");    DISPLAY_INT2(USART3, d.Year, 16, 0);
  USART_puts(USART3, " HOUR -> ");    DISPLAY_INT2(USART3, t.Hours, 16, 0);
  USART_puts(USART3, " MINUTES -> "); DISPLAY_INT2(USART3, t.Minutes, 16, 0);
  USART_puts(USART3, " SECONDS -> "); DISPLAY_INT2(USART3, t.Seconds, 16, 0);
#endif
}

// Set only the minutes (binary value), keep hours and seconds.
void RTC_SetMinutes(uint8_t minutes_bin)
{
  RTC_TimeTypeDef t;
  RTC_DateTypeDef d;
  HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
  P_RTC_SetTimeDate(t.Hours, minutes_bin, t.Seconds, RTC_FORMAT_BIN);
}

// Set only the hours (BCD value), keep minutes and seconds.
void RTC_SetHours(uint8_t hours_bcd)
{
  RTC_TimeTypeDef t;
  RTC_DateTypeDef d;
  HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BCD);
  HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BCD);
  P_RTC_SetTimeDate(hours_bcd, t.Minutes, t.Seconds, RTC_FORMAT_BCD);
}

// Interactive time entry over USART3 ("*<v>#" frames).
void RTC_Update(void)
{
  uint8_t date, month, year, hours, minutes;
  int status;

  STOP_TIMER(RTC_TIM_TYPE);

  USART_puts(USART3, "PLEASE ENTER DATE -> "); eol(USART3);
  do { status = TIMER_GET_VAL(5); } while (status != 1);
  date = inbuf[1];

  USART_puts(USART3, "PLEASE ENTER MONTH -> "); eol(USART3);
  do { status = TIMER_GET_VAL(5); } while (status != 1);
  month = inbuf[1];

  USART_puts(USART3, "PLEASE ENTER YEAR -> "); eol(USART3);
  do { status = TIMER_GET_VAL(5); } while (status != 1);
  year = inbuf[1];

  USART_puts(USART3, "PLEASE ENTER HOUR -> "); eol(USART3);
  do { status = TIMER_GET_VAL(5); } while (status != 1);
  hours = inbuf[1];

  USART_puts(USART3, "PLEASE ENTER MINUTES -> "); eol(USART3);
  do { status = TIMER_GET_VAL(5); } while (status != 1);
  minutes = inbuf[1];

  {
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};
    t.Hours = hours;
    t.Minutes = minutes;
    HAL_RTC_SetTime(&hrtc, &t, RTC_FORMAT_BCD);
    d.WeekDay = RTC_WEEKDAY_TUESDAY;
    d.Date = date;
    d.Month = month;
    d.Year = year;
    HAL_RTC_SetDate(&hrtc, &d, RTC_FORMAT_BCD);
  }

  TIMER_UPDATE = 0;
  INIT_TIMER_CONFIG(RTC_TIMER_PERIPH, RTC_TIMER_IRQ, RTC_TIM_TYPE, RTC_UPDATE_TIME);
}

// Wait for a "*<value>#" frame in inbuf (filled by the USART ISR).
int TIMER_GET_VAL(unsigned char trial)
{
  (void)trial;
  int status;

  inbuf[0] = 0xFF;
  inbuf[1] = 0xFF;
  inbuf[2] = 0xFF;
  inbuf[3] = 0xFF;

  while (inbuf[1] != '*' && inbuf[3] != '#')
  {
  }

  if (inbuf[2] >= 0 && inbuf[2] <= 60)
    status = 0;
  else
    status = 1;
  return status;
}

uint8_t ByteToBcd(uint8_t Value)
{
  uint8_t bcdhigh = 0;
  while (Value >= 10)
  {
    bcdhigh++;
    Value -= 10;
  }
  return (uint8_t)((bcdhigh << 4) | Value);
}

uint8_t Bcd2ToByte(uint8_t Value)
{
  uint8_t tmp = (uint8_t)(((Value & 0xF0u) >> 4) * 10u);
  return (uint8_t)(tmp + (Value & 0x0Fu));
}

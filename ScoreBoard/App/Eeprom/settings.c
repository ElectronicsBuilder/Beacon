/*
 * settings.c - user settings kept in the EEPROM (brightness, display mode).
 *
 * Changes are saved 2 s after the last one, so holding a key does not wear the
 * EEPROM, and only bytes that differ are written. A marker byte tells a blank
 * EEPROM (0xFF) apart from saved settings; without it the defaults are used.
 */
#include "settings.h"
#include "eeprom_24lc.h"
#include "matrix.h"
#include "cmsis_os.h"

#define SETTINGS_ADDR_MARKER      0x0F00u
#define SETTINGS_ADDR_BRIGHTNESS  0x0F01u
#define SETTINGS_ADDR_MODE        0x0F02u
#define SETTINGS_ADDR_RTC_TRIM    0x0F03u
#define SETTINGS_ADDR_CD_DAYS     0x0F04u
#define SETTINGS_ADDR_CU_DAYS     0x0F05u
#define SETTINGS_MARKER           0xB7u
#define SETTINGS_SAVE_DELAY_MS    2000u

static uint8_t  save_pending;
static uint32_t save_at_ms;
static int      saved_mode = -1;
static uint8_t  mode_value = 0xFFu;
static uint8_t  trim_value = 0xFFu;
static uint8_t  days_value[2] = { 0xFFu, 0xFFu };   /* countdown, count-up */

void Settings_Load(void)
{
  int16_t marker = Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_MARKER);
  if (marker != SETTINGS_MARKER)
  {
    return;
  }
  int16_t value = Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_BRIGHTNESS);
  if (value >= MATRIX_BRIGHTNESS_MIN && value <= 100)
  {
    Matrix_SetBrightness((uint8_t)value);
  }
  int16_t trim = Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_RTC_TRIM);
  if (trim >= 0) trim_value = (uint8_t)trim;
  int16_t days = Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_CD_DAYS);
  if (days >= 0) days_value[0] = (uint8_t)days;
  days = Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_CU_DAYS);
  if (days >= 0) days_value[1] = (uint8_t)days;
  int16_t mode = Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_MODE);
  if (mode >= 0 && mode < 0xFF)
  {
    saved_mode = mode;
    mode_value = (uint8_t)mode;
  }
}

static void P_Changed(void)
{
  save_pending = 1;
  save_at_ms = osKernelGetTickCount() + SETTINGS_SAVE_DELAY_MS;
}

void Settings_BrightnessChanged(void)
{
  P_Changed();
}

int Settings_GetMode(void)
{
  return saved_mode;
}

void Settings_SetMode(uint8_t mode)
{
  mode_value = mode;
  P_Changed();
}

int Settings_GetRtcTrim(void)
{
  return trim_value != 0u;
}

void Settings_SetRtcTrim(uint8_t on)
{
  trim_value = on ? 1u : 0u;
  P_Changed();
}

int Settings_GetDays(int countup)
{
  return days_value[countup ? 1 : 0] == 1u;     /* default (0xFF) off: the big digits show the days */
}

void Settings_SetDays(int countup, uint8_t on)
{
  days_value[countup ? 1 : 0] = on ? 1u : 0u;
  P_Changed();
}

static void P_Write(uint16_t addr, uint8_t value)
{
  if (Eeprom24_ReadByte(EE24_ADDR_0, addr) != value)
  {
    Eeprom24_WriteByte(EE24_ADDR_0, addr, value);
  }
}

void Settings_Poll(void)
{
  if (save_pending && (int32_t)(osKernelGetTickCount() - save_at_ms) >= 0)
  {
    save_pending = 0;
    P_Write(SETTINGS_ADDR_BRIGHTNESS, Matrix_GetBrightness());
    P_Write(SETTINGS_ADDR_MODE, mode_value);
    P_Write(SETTINGS_ADDR_RTC_TRIM, trim_value);
    P_Write(SETTINGS_ADDR_CD_DAYS, days_value[0]);
    P_Write(SETTINGS_ADDR_CU_DAYS, days_value[1]);
    P_Write(SETTINGS_ADDR_MARKER, SETTINGS_MARKER);
  }
}

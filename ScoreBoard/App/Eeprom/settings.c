/*
 * settings.c - user settings kept in the EEPROM (currently: matrix brightness).
 *
 * The brightness is saved 2 s after the last +/- key press, so holding the key
 * does not wear the EEPROM. A marker byte tells a blank EEPROM (0xFF) apart
 * from saved settings; without it the compiled-in default is used.
 */
#include "settings.h"
#include "eeprom_24lc.h"
#include "matrix.h"
#include "cmsis_os.h"

#define SETTINGS_ADDR_MARKER      0x0F00u
#define SETTINGS_ADDR_BRIGHTNESS  0x0F01u
#define SETTINGS_MARKER           0xB7u
#define SETTINGS_SAVE_DELAY_MS    2000u

static uint8_t  save_pending;
static uint32_t save_at_ms;

void Settings_Load(void)
{
  int16_t marker = Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_MARKER);
  int16_t value  = Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_BRIGHTNESS);

  if (marker == SETTINGS_MARKER && value >= MATRIX_BRIGHTNESS_MIN && value <= 100)
  {
    Matrix_SetBrightness((uint8_t)value);
  }
}

void Settings_BrightnessChanged(void)
{
  save_pending = 1;
  save_at_ms = osKernelGetTickCount() + SETTINGS_SAVE_DELAY_MS;
}

void Settings_Poll(void)
{
  if (save_pending && (int32_t)(osKernelGetTickCount() - save_at_ms) >= 0)
  {
    save_pending = 0;
    uint8_t value = Matrix_GetBrightness();
    if (Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_BRIGHTNESS) != value)
    {
      Eeprom24_WriteByte(EE24_ADDR_0, SETTINGS_ADDR_BRIGHTNESS, value);
    }
    if (Eeprom24_ReadByte(EE24_ADDR_0, SETTINGS_ADDR_MARKER) != SETTINGS_MARKER)
    {
      Eeprom24_WriteByte(EE24_ADDR_0, SETTINGS_ADDR_MARKER, SETTINGS_MARKER);
    }
  }
}

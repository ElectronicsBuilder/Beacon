/*
 * i2c_selftest.h - one-shot I2C / EEPROM hardware test, printed on USART3.
 *
 * Enabled with APP_I2C_SELFTEST (1 = run at boot, before the OLED init).
 * Set it to 0 once the EEPROM wiring is confirmed.
 */
#ifndef APP_I2C_SELFTEST_H
#define APP_I2C_SELFTEST_H

#ifndef APP_I2C_SELFTEST
#define APP_I2C_SELFTEST 0   /* set to 1 to run the EEPROM wiring test at boot */
#endif

void I2C_SelfTest(void);

#endif

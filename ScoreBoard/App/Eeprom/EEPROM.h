#ifndef _EEPROM_H
#define _EEPROM_H

#include "stm32f4xx.h"

#include "app_main.h"
#include "usart.h"
#include "app_hal.h"
#include "app_hal.h"
#include "app_hal.h"
#include "app_hal.h"

#include "ee_i2c.h"
#include "GAME.h"



//EEPROM SESSION



#define GPIO_TIMEOUT      (0x10000)  // timeout for blinking LEDs
#define I2C_EEPROM_ADDR   (0xA0)     // EEPROM I2C-Bus device address

#define I2C_EEPROM_ADDR1   (0xA0)    // EEPROM I2C-Bus device address

#define I2C_EEPROM_PAGE   (32)       // EEPROM memory page size
#define I2C_EEPROM_SINGLE   (1)


typedef enum EE_I2C
{

	i2c_init_success = 0,
	i2c_init_failed = 1,
	i2c_com_succes = 2,
	i2c_com_failed = 3,
	ee_saved = 4,
	ee_unsaved = 5,
	ee_store_data_comp = 6,
	ee_read_data_com = 7

}EE_I2C;







void GET_PLAYER_DATA(void);
void SAVE_PLAYER_DATA(void);
void EE_TEST(void);
uint16_t  CHECK_SAVED(void);
void read_from_eeprom(char* myString, uint16_t addr_ptr);
void store_to_eeprom(char* myString, uint16_t addr_ptr);


void store_to_eeprom_single(char myString, uint16_t addr_ptr);
void read_from_eeprom_single(char myString, uint16_t addr_ptr);

char* EE_TEST2(uint16_t loc) ;



#endif /*_`_H*/

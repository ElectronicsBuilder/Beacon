

#include "EEPROM.h"
#include "log.h"

uint8_t eeprom_ok;   /* result of EE_TEST() at boot */
#include "Ez_USART.h"
#include "i2c1_bus.h"

/* Wait (max. 20 ms) until the EEPROM acknowledges again after a write. */
static void EE_WaitReady(void)
{
  (void)I2C1Bus_WaitReady(I2C_EEPROM_ADDR, 20u);
}

#include "ee_i2c.h"

/*---------------------------- Variable Define -----------------------------*/

/*---------------------------- Function Prototypes -------------------------*/

char WRITE_CHECK[I2C_EEPROM_PAGE] = 	{"Default String"};



char SAVED_FLAG[] = { 'S', 'A', 'V', 'E', 'D'};


void store_to_eeprom(char* myString, uint16_t addr_ptr)
{
 // printf("Write String: >%s< to EEPROM-Addr[0x%04X]\n", myString, addr_ptr);
  EE_WaitReady();   // wait until the EEPROM has finished its last write
  I2C_WriteTransfer(I2C_EEPROM_ADDR, (uint8_t *)myString, strlen(myString)+1, addr_ptr, sizeof(addr_ptr));
}


void read_from_eeprom(char* myString, uint16_t addr_ptr)
{
  EE_WaitReady();   // wait until the EEPROM has finished its last write
  I2C_ReadTransfer(I2C_EEPROM_ADDR, (uint8_t *)myString, I2C_EEPROM_PAGE, addr_ptr, sizeof(addr_ptr));
//  printf("Read String: >%s< from  EEPROM-Addr[0x%04X]\n", myString, addr_ptr);
}



uint16_t  CHECK_SAVED(void)
{
	/* "SAVED" marker at address 0 tells whether the EEPROM was initialised */
	read_from_eeprom(WRITE_CHECK, 0x0000);
	return (memcmp(WRITE_CHECK, SAVED_FLAG, sizeof SAVED_FLAG) == 0) ? ee_saved : ee_unsaved;
}

void EE_TEST(void)
{
	 I2C_BusInit();

	  if (I2C_CheckDevice(I2C_EEPROM_ADDR) != OK)
	  {

	    eeprom_ok = 0;
	    LOG_E("EEPROM (I2C 0x%02X) not responding", I2C_EEPROM_ADDR);





	  }


	  else{

		  eeprom_ok = 1;
		  LOG_I("EEPROM ok");

		   //store_to_eeprom_single(0, 10)
		 // store_to_eeprom("BEST PLAYER", 120);
	  }



}













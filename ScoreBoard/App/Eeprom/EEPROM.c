

#include "EEPROM.h"
#include "log.h"
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

char myString[I2C_EEPROM_PAGE] = 		{"Default String"};
char myString1[I2C_EEPROM_PAGE] = 		{"Default String"};
char myString2[I2C_EEPROM_PAGE] = 		{"Default String"};
char myString3[I2C_EEPROM_PAGE] = 		{"Default String"};
char WRITE_CHECK[I2C_EEPROM_PAGE] = 	{"Default String"};

char TEST_STRING[I2C_EEPROM_PAGE];// = 	{"Default String"};

char * TEST_STRING_H;

char SAVED_FLAG[] = { 'S', 'A', 'V', 'E', 'D'};


void store_to_eeprom(char* myString, uint16_t addr_ptr)
{
 // printf("Write String: >%s< to EEPROM-Addr[0x%04X]\n", myString, addr_ptr);
  EE_WaitReady();   // wait until the EEPROM has finished its last write
  I2C_WriteTransfer(I2C_EEPROM_ADDR, myString, strlen(myString)+1, addr_ptr, sizeof(addr_ptr));
}

void store_to_eeprom_single(char data, uint16_t addr_ptr)
{
 // printf("Write String: >%s< to EEPROM-Addr[0x%04X]\n", myString, addr_ptr);
  EE_WaitReady();   // wait until the EEPROM has finished its last write
  I2C_WriteTransfer_single(I2C_EEPROM_ADDR, data, 1, addr_ptr, sizeof(addr_ptr));
}

void read_from_eeprom(char* myString, uint16_t addr_ptr)
{
  EE_WaitReady();   // wait until the EEPROM has finished its last write
  I2C_ReadTransfer(I2C_EEPROM_ADDR, myString, I2C_EEPROM_PAGE, addr_ptr, sizeof(addr_ptr));
//  printf("Read String: >%s< from  EEPROM-Addr[0x%04X]\n", myString, addr_ptr);
}

void read_from_eeprom_single(char data, uint16_t addr_ptr)
{

  EE_WaitReady();   // wait until the EEPROM has finished its last write
  I2C_ReadTransfer_single(I2C_EEPROM_ADDR, data, 1, addr_ptr, sizeof(addr_ptr));
//  printf("Read String: >%s< from  EEPROM-Addr[0x%04X]\n", myString, addr_ptr);
}

void ee_tx_byte(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t data )
{
	 I2C_write2(I2Cx, addr);
	 I2C_write2(I2Cx, data);

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

	    LOG_E("EEPROM (I2C 0x%02X) not responding", I2C_EEPROM_ADDR);





	  }


	  else{

		  LOG_I("EEPROM ok");

		   //store_to_eeprom_single(0, 10)
		  //  store_to_eeprom("PLAYER A", 100);
		 // store_to_eeprom("BEST PLAYER", 120);
	  }



}

char* EE_TEST2(uint16_t loc)
{
	char TEST_STRING[I2C_EEPROM_PAGE];// = 	{"Default String"};
	//read_from_eeprom(TEST_STRING, loc);
	char * TEST_STRING_H;

	// I2C_BusInit();

	//  if (I2C_CheckDevice(I2C_EEPROM_ADDR) != OK)
	//  {

	 //   USART_puts(USART3, "I2C-Bus device with address is not responding\n");

	  //  DISPLAY_INT2(USART3, I2C_EEPROM_ADDR, 10, 1);
		//  return 0;
	 // }


	//  else{

		  //USART_puts(USART3, "I2C-Bus device eeprom initialized properly\n");

		//  store_to_eeprom("PLAYER A", 100);
		 // store_to_eeprom("BEST PLAYER", 120);
		  read_from_eeprom(TEST_STRING, loc);
		  TEST_STRING_H = TEST_STRING;

		  return TEST_STRING_H;
	//  }



}


void SAVE_PLAYER_DATA(void)
{
	uint16_t status = CHECK_SAVED();
	if(status == ee_saved)
	{
		/* already initialised */
	}

	else if(status == ee_unsaved)
	{

	store_to_eeprom("SAVED", 0x0000);
	store_to_eeprom("PLAYER D", 0x0020);
	store_to_eeprom("THE GAME OF THRONES", 0x0040);
	store_to_eeprom(" PLAYER E ", 0x0060);
	store_to_eeprom(" DRAGONS OF SOFT", 0x0080);
	}

}


uint16_t SAVE_PLAYER_DATA2(char * DATA, uint16_t ADDRESS)
{
	if (I2C_CheckDevice(I2C_EEPROM_ADDR) != OK)
		  {
			store_to_eeprom(DATA, ADDRESS);
			// auto increment page
			//ADDRESS += 0x20;
			return ee_store_data_comp;
		  }


		  else{

			return i2c_com_failed;
		  }


}

void GET_PLAYER_DATA2(char* DATA, uint16_t ADDRESS)
{
	if (I2C_CheckDevice(I2C_EEPROM_ADDR) != OK)
		  {
			read_from_eeprom(DATA, ADDRESS);
			return ee_read_data_com;
		  }

		  else{

			return i2c_com_failed;
		  }


}


void GET_PLAYER_DATA(void)
{
/*
		 read_from_eeprom(myString, 0x00100);
		 PLAYER1.name = myString;

		 read_from_eeprom(myString1, 0x00120);
		 PLAYER1.message = myString1;

		 read_from_eeprom(myString2, 0x00160);
		 PLAYER2.name = myString2;
		 read_from_eeprom(myString3, 0x00180);
		 PLAYER2.message = myString3;
		 */
}






#include "GAME.h"
#include "Ez_USART.h"
#include "RANDOM_NUMBER.h"
#include "EEPROM.h"
#include "eeprom_24lc.h"
//#define SIM_EE_TRANSACTION


char EE_SIM[500][32];
uint16_t id;
#define PLAYER_DATA_START_ADDR 0x0000
uint16_t  GAMERS_START_ADDR = 0x0100;  // make this automatic

uint16_t serv, serv_h;
unsigned char CURRENT_SERVER[] = { no_server, PLAYER_1, PLAYER_2};

//#define P_LOOKUP
//#define SIMULATE_EE
//#define DEBUG_GAME

//#define DEBUG_DISPLAY_PLAYER
//#define DEBUG_GAME_AT_SCORES

void  RETURN_RANDOM_SERVER(void)
{

	RNG_Init();
	serv = GET_RANDOM();
	//return serv;
}

uint16_t RETURN_GAME_SERVER(void)
{
	//RNG_Init();
	serv = GET_RANDOM();

	return serv;
}

GAMERS PORT_GAMER_FROM_ID(uint16_t id)
{

	GAMER_ALL[0] = GAMER0;
	GAMER_ALL[1] = GAMER1;
	GAMER_ALL[2] = GAMER2;
	GAMER_ALL[3] = GAMER3;
	GAMER_ALL[4] = GAMER4;
	GAMER_ALL[5] = GAMER5;
	GAMER_ALL[6] = GAMER6;
	GAMER_ALL[7] = GAMER7;
	GAMER_ALL[8] = GAMER8;
	GAMER_ALL[9] = GAMER9;



	return GAMER_ALL[id];
}

#ifdef SIM_EE_TRANSACTION
void store_to_eeprom(char *DATA, uint16_t address)
{
	//EE_SIM[address][0]= DATA;

	for(i = 0; i <= I2C_EEPROM_PAGE; i++)
	{
		EE_SIM[address][i] = DATA[i];
	}

}

void read_from_eeprom(char*DATA, uint16_t address)
{
	for(i = 0; i <= I2C_EEPROM_PAGE; i++)
	{
		DATA[i] = EE_SIM[address][i];
	}
}

#endif

GAMERS UPDATE_GAMER_DATA(GAMERS GAMERX, GAMERS GAMERY)
{
	/*
	//memcpy(&GAMERY, &GAMERX, sizeof(GAMERY));
	GAMERY.id = GAMERX.id;
	GAMERY.current_score = GAMERX.current_score;
	GAMERY.score = GAMERX.score;
	GAMERY.Nserve = GAMERX.Nserve;
	GAMERY.PD_loc = GAMERX.PD_loc;
	GAMERY.name = GAMERX.name;
	GAMERY.message = GAMERX.message;
	return GAMERY;
	*/
}

GAMERS RETURN_SAVE_LOC(uint16_t id, GAMERS GAMERX)
{
#ifdef P_LOOKUP
	GAMERX = PORT_GAMER_FROM_ID(id);
#endif
	//for testing out of ee//

#ifdef SIMULATE_EE
	uint16_t a = id *2;
	uint16_t start_loc = a  + (id - 2);

	GAMERX.name_loc = start_loc;
	GAMERX.message_loc = start_loc+1;
	GAMERX.PD_loc = start_loc+2;

	DISPLAY_INT2(USART3,GAMERX.name_loc, 10, 0);
	DISPLAY_INT2(USART3,GAMERX.message_loc, 10, 0);
	DISPLAY_INT2(USART3,GAMERX.PD_loc, 10, 0);

	//GAMER GAMERY = UPDATE_GAMER_DATA(GAMERY, GAMERX);

	return GAMERX;
#endif

#ifndef SIMULATE_EE
	// y = mx + b -> (1, 100) (2, 160) -> y = 60x + 40//
/*
	uint16_t start_loc = (80 * id) + 20 ; //+ PLAYER_DATA_START_ADDR;
	GAMERX.name_loc = start_loc;
	GAMERX.message_loc = start_loc+20;
	GAMERX.PD_loc = start_loc+40;
	GAMERX.score_data_loc = start_loc+60;
*/
	uint16_t start_loc = (128 * id) + 32 ; //+ PLAYER_DATA_START_ADDR;
	GAMERX.name_loc = start_loc;
	GAMERX.message_loc = start_loc+ 32;
	GAMERX.PD_loc = start_loc+ 64;
	GAMERX.score_data_loc = start_loc+ 96;

#ifdef DEBUG_DISPLAY_PLAYER
	DISPLAY_INT2(USART3,GAMERX.name_loc, 10, 0);			USART_puts(USART3, " -> ");
	DISPLAY_INT2(USART3,GAMERX.message_loc, 10, 0);			USART_puts(USART3, " -> ");
	DISPLAY_INT2(USART3,GAMERX.PD_loc, 10, 0);				USART_puts(USART3, " -> ");
	DISPLAY_INT2(USART3,GAMERX.score_data_loc, 10, 0);
#endif
	return GAMERX;

#endif

}



uint16_t RETURN_SCORE_LOC(uint16_t id)
{

	uint16_t score_loc = (32*id) + 2560;
	eol(USART3);
	USART_puts(USART3, " STORAGE LOC of ID  ");DISPLAY_INT2(USART3, id, 10, 0);USART_puts(USART3, "  -> ");
	DISPLAY_INT2(USART3, score_loc, 10, 0);
	eol(USART3);
	return score_loc;
}

GAMERS SAVING_GAMER_DATA(GAMERS GAMERX)
{
	store_to_eeprom(GAMERX.name, 0);
	store_to_eeprom(GAMERX.message, 1);
	store_to_eeprom(GAMERX.PLAYER_DATA, 2);

	return GAMERX;
}

GAMERS SAVING_GAMER_DATA2(uint16_t id, GAMERS GAMERX)
{
#ifdef P_LOOKUP
	GAMERX = PORT_GAMER_FROM_ID(id);
#endif

	uint16_t status = CHECK_SAVED();			// NEED TO FIGURE THIS OUT LATER. WHETHER THIS FLAG GOES HIGH WHEN GAMER DATA ARE XFER. IDK. @*f*h
	//DISPLAY_INT2(USART3, status, 10, 1);
	if(status == ee_saved)
	{
		USART_puts(USART3, "SAVED STAT DETECTED ");
	}

	else
	{
		 store_to_eeprom(GAMERX.name, GAMERX.name_loc);
		 store_to_eeprom(GAMERX.message, GAMERX.message_loc);
		 store_to_eeprom(GAMERX.PLAYER_DATA, GAMERX.PD_loc);

		 store_to_eeprom(GAMERX.SAVED_SCORE_DATA, GAMERX.score_data_loc);


	}
	// GAMERY = UPDATE_GAMER_DATA(GAMERY, GAMERX);

	return GAMERX;
}

GAMERS SAVING_GAMER_DATA_INIT(uint16_t id, GAMERS GAMERX)
{
#ifdef P_LOOKUP
	GAMERX = PORT_GAMER_FROM_ID(id);
#endif

	uint16_t status = CHECK_SAVED();			// NEED TO FIGURE THIS OUT LATER. WHETHER THIS FLAG GOES HIGH WHEN GAMER DATA ARE XFER. IDK. @*f*h
	//DISPLAY_INT2(USART3, status, 10, 1);
	if(status == ee_saved)
	{
		USART_puts(USART3, "SAVED STAT DETECTED ");
	}

	else
	{
		 store_to_eeprom(GAMERX.name, GAMERX.name_loc);
		 store_to_eeprom(GAMERX.message, GAMERX.message_loc);
		 store_to_eeprom(GAMERX.PLAYER_DATA, GAMERX.PD_loc);
		 store_to_eeprom(GAMERX.SAVED_SCORE_DATA, GAMERX.score_data_loc);


	}
	// GAMERY = UPDATE_GAMER_DATA(GAMERY, GAMERX);

	return GAMERX;
}

GAMERS SAVING_GAMER_DATA_SCORE(uint16_t id, GAMERS GAMERX)
{
#ifdef P_LOOKUP
	GAMERX = PORT_GAMER_FROM_ID(id);
#endif

	eol(USART3);
	USART_puts(USART3, "SAVING ->  ");
	DISPLAY_INT2(USART3, id, 10, 1);

	USART_puts(USART3, "@ LOCATION -> ");
	DISPLAY_INT2(USART3,GAMERX.score_data_loc, 10, 1);

	uint16_t status = CHECK_SAVED();			// NEED TO FIGURE THIS OUT LATER. WHETHER THIS FLAG GOES HIGH WHEN GAMER DATA ARE XFER. IDK. @*f*h
	//DISPLAY_INT2(USART3, status, 10, 1);
	if(status == ee_saved)
	{
		USART_puts(USART3, "SAVED STAT DETECTED ");
	}

	else
	{
		// store_to_eeprom(GAMERX.name, GAMERX.name_loc);
		// store_to_eeprom(GAMERX.message, GAMERX.message_loc);
		// store_to_eeprom(GAMERX.PLAYER_DATA, GAMERX.PD_loc);
		 store_to_eeprom(GAMERX.SAVED_SCORE_DATA, GAMERX.score_data_loc);


	}

	return GAMERX;
}

GAMERS text_player_data(GAMERS GAMERX, uint16_t id)
{
	//GAMERS GAMERY;

	uint16_t m;


#ifdef P_LOOKUP
	 GAMERX = PORT_GAMER_FROM_ID(id);
#endif

	if(id == 1)
	{
		GAMERX.id = id;
		//GAMERX.name = "PLAYER A";
		strcpy(GAMERX.name, "PLAYER A");
		//GAMERX.message = "BOW DOWN TO THE KING";
		strcpy(GAMERX.message,"BOW DOWN TO THE KING" );
	}
	else if(id == 2)
	{
		GAMERX.id = id;
		//GAMERX.name = " PLAYER E ";
		strcpy(GAMERX.name, " PLAYER E ");
		//GAMERX.message = "GREATEST PLAYER EVER";
		strcpy(GAMERX.message,"PING PONG EXTRAORDINAIRE " );
	}

	else if(id == 3)
	{
		GAMERX.id = id;
		//GAMERX.name = " PLAYER B ";
		strcpy(GAMERX.name, "B ZELLE");
		//GAMERX.message = "SERVING SINCE 1985";
		strcpy(GAMERX.message,"NETTING SERVES SINCE 2015" );

	}

	else if(id == 4)
	{
		GAMERX.id = id;
		//GAMERX.name = " PLAYER C ";
		strcpy(GAMERX.name, " PLAYER C ");
		//GAMERX.message = "ONCE A LOOSER ALWAYS A LOOSER";
		strcpy(GAMERX.message,"CHAMPION OF THE UNIVERSE" );
	}

	else if(id == 5)
	{
		GAMERX.id = id;
		strcpy(GAMERX.name, " DEREK ");
		strcpy(GAMERX.message,"SHAZAM" );
	}

	else if(id == 6)
	{
		GAMERX.id = id;
		strcpy(GAMERX.name, " SEAN ");
		strcpy(GAMERX.message,"VINEGAR STROKES" );
	}


	else if(id == 7)
	{
		GAMERX.id = id;
		strcpy(GAMERX.name, "PLAYER1");
		strcpy(GAMERX.message,"TALK TO HAND 1" );
	}


	else if(id == 8)
	{
		GAMERX.id = id;
		strcpy(GAMERX.name, "PLAYER2");
		strcpy(GAMERX.message,"TALK TO HAND 2" );
	}

	else if(id == 0)
	{
		GAMERX.id = id;
		//GAMERX.name = " COMP ";
		//GAMERX.message = "COMPUTER PLAYER";
		strcpy(GAMERX.name, " COMP ");
		strcpy(GAMERX.message,"COMPUTER PLAYER" );
	}

	else if(id == 9)
	{
		GAMERX.id = id;
		//GAMERX.name = " COMP ";
		//GAMERX.message = "COMPUTER PLAYER";
		strcpy(GAMERX.name, "CHARITY");
		strcpy(GAMERX.message,"TALK TO THE HAND 3" );
	}
	else if(id == 10)
	{
		GAMERX.id = id;
		//GAMERX.name = " COMP ";
		//GAMERX.message = "COMPUTER PLAYER";
		strcpy(GAMERX.name, " PHIL");
		strcpy(GAMERX.message,"DUNK MASTER" );
	}


	//GAMERX.ALL_TIME_SC[GAMERX.id][GAMERX.id][0] = 15;

	//DISPLAY_INT2(USART3, GAMER1.ALL_TIME_SC[GAMER1.id][GAMER2.id][0], 10, 1);

	//GAMERX.ALL_TIME_SC[GAMERX.id][GAMERX.id][1] = 6;
	//DISPLAY_INT2(USART3, GAMER1.ALL_TIME_SC[GAMER1.id][GAMER2.id][1], 10, 1);


//	GAMERX.PLAYER_DATA[0] = GAMERX.id;
//	GAMERX.PLAYER_DATA[1] = 1;
//	GAMERX.PLAYER_DATA[2] = 2;
//	GAMERX.PLAYER_DATA[3] = 3;

//	GAMERX.PLAYER_DATA[4] = GAMERX.ALL_TIME_SC[GAMERX.id][GAMERX.id][0];
//	GAMERX.PLAYER_DATA[5] = GAMERX.ALL_TIME_SC[GAMERX.id][GAMERX.id][1];

	for(m = 0; m < 32; m++)
	{
		GAMERX.SAVED_SCORE_DATA[m] = 0;
	}
	//strcpy(GAMERX.SAVED_SCORE_DATA, 0);


	return GAMERX;

}

void TEST_READ_DATA(uint16_t id)
{
	 char temp_tring1[I2C_EEPROM_PAGE]= 	{"Default String"};
	  read_from_eeprom(temp_tring1, 0);
	  read_from_eeprom(temp_tring1, 1);
	  read_from_eeprom(temp_tring1, 2);

	  //USART_puts(USART3, "GAMER NAME -> "); USART_puts(USART3, temp_tring1); eol(USART3);
	  //USART_puts(USART3, "GAMER MESSAGE -> ");  USART_puts(USART3, temp_tring1); eol(USART3);
	 // USART_puts(USART3, "GAMER DATA - > "); USART_puts(USART3, temp_tring1); eol(USART3);
}

GAMERS TEST_READ_DATA2(uint16_t id, GAMERS GAMERX)
{
#ifdef P_LOOKUP
	GAMERX = PORT_GAMER_FROM_ID(id);
#endif
	//GAMERX = GAMER1;

	//char temp_tring1[I2C_EEPROM_PAGE]= 	{"Default String"};

	/*
	  USART_puts(USART3, "EE SAVED AT -> ");
	  DISPLAY_INT2(USART3, GAMERX.name_loc, 10, 0);
	  DISPLAY_INT2(USART3, GAMERX.message_loc, 10, 0);
	  DISPLAY_INT2(USART3, GAMERX.PD_loc, 10, 0);
*/



//	GAMERX.name = EE_TEST2(GAMERX.name_loc);

//	GAMERX.message = EE_TEST2(GAMERX.message_loc);




	  read_from_eeprom(GAMERX.name,  GAMERX.name_loc);




	  read_from_eeprom(GAMERX.message, GAMERX.message_loc);

	  read_from_eeprom(GAMERX.PLAYER_DATA, GAMERX.PD_loc);
	//  GAMERX.PLAYER_DATA = GAMERX.TEMP_DATA;

	 read_from_eeprom(GAMERX.SAVED_SCORE_DATA, GAMERX.score_data_loc);
	//  GAMERX.SAVED_SCORE_DATA = GAMERX.TEMP_DATA;

		for(n = 0; n < 11; n++)
		{
			//Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.PD_loc + n), 0); // reset all
			GAMERX.PLAYER_DATA[n] = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.PD_loc + n));
		}

		for(n = 0; n < 11; n++)
		{

			//Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.score_data_loc + n), 0); // reset all
			GAMERX.SAVED_SCORE_DATA[n] = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.score_data_loc + n));
		}



  	  //  USART_puts(USART3, "GAMER NAME -> "); USART_puts(USART3, GAMERX.TEMP_DATA); eol(USART3);
	//  USART_puts(USART3, "GAMER MESSAGE -> ");  USART_puts(USART3, GAMERX.TEMP_DATA); eol(USART3);
	//  USART_puts(USART3, "GAMER DATA - > "); USART_puts(USART3, GAMERX.TEMP_DATA); eol(USART3);
		return GAMERX;

}

GAMERS TEST_READ_DATA2_SCORE(uint16_t id, GAMERS GAMERX)
{

			for(n = 0; n < 11; n++)
			{
				//Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.PD_loc + n), 0); // reset all
				GAMERX.PLAYER_DATA[n] = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.PD_loc + n));
			}

			for(n = 0; n < 11; n++)
			{

			//	Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.score_data_loc + n), 0); // reset all
				GAMERX.SAVED_SCORE_DATA[n] = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.score_data_loc + n));
			}

			return GAMERX;

}





void DISPLAY_PLAYER_INFO(GAMERS GAMERX)
{
#ifdef DEBUG_DISPLAY_PLAYER
	eol(USART3);
	USART_puts(USART3, "GAMER NAME -> "); USART_puts(USART3, GAMERX.name); eol(USART3);
	USART_puts(USART3, "GAMER MESSAGE -> "); USART_puts(USART3,  GAMERX.message); eol(USART3);
	USART_puts(USART3, "GAMER DATA -> "); //USART_puts(USART3, GAMERX.PLAYER_DATA); eol(USART3);

	//eol(USART3);

/*
	for(n = 0; n < 11; n++)
	{
		//Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.PD_loc + n), 0); // reset all
		GAMERX.PLAYER_DATA[n] = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.PD_loc + n));
	}
*/
    for(m = 0;  m < 11; m++ )
    {
      DISPLAY_INT2(USART3,  GAMERX.PLAYER_DATA[m], 10, 0);
      USART_puts(USART3, "--");
    }

    eol(USART3);

	USART_puts(USART3, "SCORE DATA -> "); //USART_puts(USART3, GAMERX.SAVED_SCORE_DATA);eol(USART3);

/*	for(n = 0; n < 11; n++)
	{

	//	Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.score_data_loc + n), 0); // reset all
		GAMERX.SAVED_SCORE_DATA[n] = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.score_data_loc + n));
	}
*/
    for(m = 0;  m < 11; m++ )
    {
      DISPLAY_INT2(USART3,  GAMERX.SAVED_SCORE_DATA[m], 10, 0);
      USART_puts(USART3, "--");
    }


	eol(USART3);


	//USART_puts(USART3, GAMERX.SCORE_DATA); eol(USART3);
#endif
}

void TEST_GAMERS2(GAMERS GAMERX, uint16_t id)
{

	GAMERX.id = id;
//	GAMERX = text_player_data(GAMERX, id);
	GAMERX  = RETURN_SAVE_LOC(id, GAMERX);
//	GAMERX  = SAVING_GAMER_DATA_INIT(id, GAMERX);
	GAMERX = TEST_READ_DATA2(id, GAMERX);

	GAMER_ALL[id] = GAMERX;
/*
	USART_puts(USART3, "GAMER NAME -> "); USART_puts(USART3, GAMER_ALL[id].name); eol(USART3);
	USART_puts(USART3, "GAMER MESSAGE -> "); USART_puts(USART3, GAMER_ALL[id].message); eol(USART3);
	USART_puts(USART3, "GAMER DATA -> "); USART_puts(USART3, GAMER_ALL[id].PLAYER_DATA); eol(USART3);
	*/

	DISPLAY_PLAYER_INFO(GAMER_ALL[id]);
}

void UPDATE_TEST_GAMER(GAMERS GAMERX, uint16_t id)
{

	GAMERX.id = id;
	GAMERX  = RETURN_SAVE_LOC(id, GAMERX);
	GAMERX = TEST_READ_DATA2_SCORE(id, GAMERX);

	GAMER_ALL[id] = GAMERX;

	DISPLAY_PLAYER_INFO(GAMER_ALL[id]);
}

void TEST_GAMERS3(uint16_t id)
{

	if(id == 1)
	{
		GAMER2.id = 2;

		GAMER1 = text_player_data(GAMER_ALL[id], id);
		GAMER1  = RETURN_SAVE_LOC(id, GAMER_ALL[id]);
		GAMER1  = SAVING_GAMER_DATA2(id, GAMER_ALL[id]);
		GAMER1 = TEST_READ_DATA2(id, GAMER_ALL[id]);
	}

	else if(id == 2)
	{
		GAMER1.id = 1;
		GAMER2 = text_player_data(GAMER_ALL[id], id);
		GAMER2  = RETURN_SAVE_LOC(id, GAMER_ALL[id]);
		GAMER2  = SAVING_GAMER_DATA2(id, GAMER_ALL[id]);
		GAMER2 = TEST_READ_DATA2(id, GAMER_ALL[id]);
	}

	else if(id == 3)
	{
		GAMERS PLAY1;

		PLAY1.id = 3;
		PLAY1 = text_player_data(PLAY1, id);
		PLAY1  = RETURN_SAVE_LOC(id, PLAY1);
		PLAY1  = SAVING_GAMER_DATA2(id, PLAY1);
		PLAY1 = TEST_READ_DATA2(id, PLAY1);

		GAMER_ALL[3] = PLAY1;


	}


	USART_puts(USART3, "GAMER NAME -> "); USART_puts(USART3, GAMER_ALL[3].name); eol(USART3);
	USART_puts(USART3, "GAMER MESSAGE -> "); USART_puts(USART3, GAMER_ALL[3].message); eol(USART3);
	USART_puts(USART3, "GAMER DATA -> "); USART_puts(USART3, GAMER_ALL[3].PLAYER_DATA); eol(USART3);

}

void SHIFT_PLAYERS(GAMERS GAMERX, uint16_t id, CONSOLE_STAT_T x)
{
	if(x == PLAYER_1)
	{
		PLAYER1 = GAMER_ALL[id]; //update player 1 info
		DISPLAY_PLAYER_INFO(GAMER_ALL[id]);
		return PLAYER1;
	}

	else if(x == PLAYER_2)
	{
		PLAYER2 = GAMER_ALL[id]; //update player 2 info
		DISPLAY_PLAYER_INFO(GAMER_ALL[id]);
		return PLAYER2;
	}


}

void TEST_GAMERS(void)
{

}

void EXTR_PLAYER_DATA(void)
{
	GAMER1 = GAMER_ALL[1];
	GAMER2 = GAMER_ALL[2];

	PLAYER1 = GAMER1;
	PLAYER2 = GAMER2;
	//DISPLAY_PLAYER_INFO(GAMER1);

	//PLAYER1.name = GAMER1.name;
	//PLAYER1.message = GAMER1.message;

	//PLAYER2.name = GAMER2.name;
	//PLAYER2.message = GAMER2.message;

}

void DISPLAY_SERVER(uint16_t SERVER)
{
	if(SERVER == PLAYER_1)
	{
		USART_puts(USART3, PLAYER1.name);
	}

	if(SERVER == PLAYER_2)
	{
		USART_puts(USART3, PLAYER2.name);
	}

}

void DISPLAY_PLAYER(uint16_t NAME)
{
	if(NAME == PLAYER_1)
	{
		USART_puts(USART3, PLAYER1.name);
	}

	if(NAME == PLAYER_2)
	{
		USART_puts(USART3, PLAYER2.name);
	}

}

void CHECK_UPDATE_SCORE(GAMERS GAMERX, GAMERS GAMERY)
{
  uint16_t m, n;

  GAMER_ALL[GAMERX.id] = GAMERX;
  GAMER_ALL[GAMERY.id] = GAMERY;

  GAMERX  = SAVING_GAMER_DATA2(GAMERX.id, GAMERX);

  GAMERY  = SAVING_GAMER_DATA2(GAMERY.id, GAMERX);


  GAMERX = TEST_READ_DATA2(GAMERX.id, GAMERX);
  GAMERY = TEST_READ_DATA2(GAMERY.id, GAMERY);

 // DISPLAY_PLAYER_INFO(GAMER_ALL[GAMERX.id]);

//  DISPLAY_PLAYER_INFO(GAMER_ALL[GAMERY.id]);


  eol(USART3);

  DISPLAY_INT2(USART3, GAMERX.SAVED_SCORE_DATA[GAMERY.id], 10, 0);
  USART_puts(USART3, " -->");
  for(m = 0; m < I2C_EEPROM_PAGE; m++)
  {
    USART_putc(USART3, GAMERX.SAVED_SCORE_DATA[m]);
  }

  eol(USART3);

  USART_puts(USART3, " ** ");

  eol(USART3);


  DISPLAY_INT2( USART3, GAMERY.SAVED_SCORE_DATA[GAMERX.id] , 10, 0);
  USART_puts(USART3, " -->");

  for(n = 0; n < I2C_EEPROM_PAGE; n++)
  {
    USART_putc(USART3, GAMERY.SAVED_SCORE_DATA[n]);
  }



}

GAMERS SAVE_UPDATE(uint16_t id, GAMERS GAMERT)
{
	//GAMERX = text_player_data(GAMERX, id);
	GAMERT  = RETURN_SAVE_LOC(id, GAMERT);
	GAMERT  = SAVING_GAMER_DATA_INIT(id, GAMERT);
	GAMERT = TEST_READ_DATA2(id, GAMERT);
}

GAMERS UPDATE_RETURN_SCORE(GAMERS GAMERX, GAMERS GAMERY)
{
	uint8_t score_track, t_data;

	   score_track = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.PD_loc + GAMERY.id));
	   score_track ++;
	   Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.PD_loc + GAMERY.id), score_track);


	   GAMERX.PLAYER_DATA[GAMERY.id] = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.PD_loc + GAMERY.id));

#ifdef DEBUG_GAME_AT_SCORES
	   eol(USART3); USART_puts(USART3, " WINNER SCORE  - >  ");
	   DISPLAY_INT2(USART3,   GAMERX.PLAYER_DATA[GAMERY.id], 10, 1);
#endif

	  // TEST_GAMERS2(GAMER_ALL[GAMERX.id], GAMERX.id);
	  // TEST_GAMERS2(GAMER_ALL[GAMERY.id], GAMERY.id);

	   //GAMERX.PLAYER_DATA[GAMERY.id] = t_data;

	   return GAMERX;

}

void DISPLAY_CURRENT_SCORE(GAMERS GAMERX, GAMERS GAMERY)
{
uint16_t m, c_id;

  uint8_t data, data2, score_track2;

#ifdef DEBUG_GAME_AT_SCORES
  eol(USART3);
  USART_puts(USART3, " OLD SCORE ");
  DISPLAY_INT2(USART3, GAMERX.SAVED_SCORE_DATA[GAMERY.id], 10, 0);

  USART_puts(USART3, " -- ");

  DISPLAY_INT2( USART3, GAMERY.SAVED_SCORE_DATA[GAMERX.id], 10, 0);
  eol(USART3);
#endif


  if(GAMERX.SAVED_SCORE_DATA[GAMERY.id] < ALL_TIME_SC[GAMERX.id][GAMERY.id][0])
  {
	  GAMERX.SAVED_SCORE_DATA[GAMERY.id] = ALL_TIME_SC[GAMERX.id][GAMERY.id][0];

	  Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.score_data_loc + GAMERY.id), GAMERX.SAVED_SCORE_DATA[GAMERY.id]);

	  data=Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.score_data_loc + GAMERY.id));

  }

  if(GAMERY.SAVED_SCORE_DATA[GAMERX.id] < ALL_TIME_SC[GAMERX.id][GAMERY.id][1])
  {
  	  GAMERY.SAVED_SCORE_DATA[GAMERX.id] = ALL_TIME_SC[GAMERX.id][GAMERY.id][1];

  	  Eeprom24_WriteByte(EE24_ADDR_0,(GAMERY.score_data_loc + GAMERX.id), GAMERY.SAVED_SCORE_DATA[GAMERX.id]);

  	  data2=Eeprom24_ReadByte(EE24_ADDR_0, (GAMERY.score_data_loc + GAMERX.id));
  }

#ifdef DEBUG_GAME_AT_SCORES
  eol(USART3);
  USART_puts(USART3, GAMERX.name);
  USART_puts(USART3, " VS ");
  USART_puts(USART3, GAMERY.name);
  eol(USART3);



  eol(USART3);

  DISPLAY_INT2(USART3, GAMERX.SAVED_SCORE_DATA[GAMERY.id], 10, 0);

  USART_puts(USART3, " -- ");

  DISPLAY_INT2( USART3, GAMERY.SAVED_SCORE_DATA[GAMERX.id], 10, 0);
  eol(USART3);
#endif


  //--------------new stuff--------


  GAMERX = UPDATE_RETURN_SCORE(GAMERX, GAMERY);


#ifdef DEBUG_GAME_AT_SCORES
  eol(USART3); USART_puts(USART3, " PLAYER1  - >  ");
  DISPLAY_INT2(USART3, data, 10, 1);
  eol(USART3);USART_puts(USART3, " PLAYER2 - >  ");
  DISPLAY_INT2(USART3, data2, 10, 1);
#endif

  UPDATE_TEST_GAMER(GAMER_ALL[GAMERX.id], GAMERX.id);
  UPDATE_TEST_GAMER(GAMER_ALL[GAMERY.id], GAMERY.id);

}





void SAVE_PLAYER_SCORE(GAMERS GAMERX)
{


}

void RESET_SCORE(void)
{
	PLAYER1.score = 0;
	PLAYER2.score = 0;
	PLAYER1.Nserve = 0;
	PLAYER2.Nserve = 0;
	GAME.POINTS = 0;

}

void GAME_TYPE_1(void)
{

	if(GAME.POINTS >=2)
	{
		GAME.POINTS = 0;
		serv++;
	}

	if(GAME.POINTS <= 0)
	{
		GAME.POINTS = 0;
	}
	if(serv > 2)
	{
		serv = 1;
	}

	//serv_h = serv; // holding server position

	GAME.SERVER = CURRENT_SERVER[serv];
//	uint16_t server2 = CURRENT_SERVER[serv];


#ifdef DEBUG_GAME

	eol(USART3);
	USART_puts(USART3, "Game points ->");
	DISPLAY_INT2(USART3, GAME.POINTS, 10, 0);

	eol(USART3);
	USART_puts(USART3, "serv ->");
	DISPLAY_INT2(USART3, serv, 10, 0);

	eol(USART3);
	USART_puts(USART3, "game server ->");
	DISPLAY_INT2(USART3, GAME.SERVER, 10, 0);

#endif


	if(PLAYER1.score > PLAYER2.score && PLAYER1.score == 10)
	{
		GAME.SERVER = PLAYER_2;
	}

	if(PLAYER2.score > PLAYER1.score && PLAYER2.score == 10)
	{
		GAME.SERVER = PLAYER_1;
	}


	if(PLAYER1.score >= PLAYER2.score + 2 && PLAYER1.score >= 11)
	{

		//GAMER1 = PLAYER1;
		   ALL_TIME_SC[PLAYER1.id][PLAYER2.id][0] = PLAYER1.score;
		   ALL_TIME_SC[PLAYER1.id][PLAYER2.id][1] = PLAYER2.score;
/*
		PLAYER1.score = 0;
		PLAYER2.score = 0;
		PLAYER1.Nserve = 0;
		PLAYER2.Nserve = 0;
*/

		GAME.STATUS = stop;
		GAME.WINNER = PLAYER_1;
		GAME.WINNER_SERVED = PLAYER1.Nserve;
		GAME.MESSAGE = PLAYER1.message;
		GAME.WINNER_NAME = PLAYER1.name;
		GAME.LOSER_NAME = PLAYER2.name;
		GAME.TOGGLE_WINNER = 1;
		//UPDATE_CURRENT_WINNER();



		  DISPLAY_CURRENT_SCORE(PLAYER1, PLAYER2);


	}


	if(PLAYER2.score >= PLAYER1.score + 2 && PLAYER2.score >= 11)
	{


				//GAMER2 = PLAYER2;
		   ALL_TIME_SC[PLAYER2.id][PLAYER1.id][0] = PLAYER2.score;
		   ALL_TIME_SC[PLAYER2.id][PLAYER1.id][1] = PLAYER1.score;

		   /*
				PLAYER1.score = 0;
				PLAYER2.score = 0;
				PLAYER1.Nserve = 0;
				PLAYER2.Nserve = 0;s

*/

				GAME.STATUS = stop;
				GAME.WINNER = PLAYER_2;
				GAME.WINNER_SERVED = PLAYER2.Nserve;
				GAME.MESSAGE = PLAYER2.message;
				GAME.WINNER_NAME = PLAYER2.name;
				GAME.LOSER_NAME = PLAYER1.name;
				GAME.TOGGLE_WINNER = 1;
				//UPDATE_CURRENT_WINNER();



				    DISPLAY_CURRENT_SCORE(PLAYER2, PLAYER1);

	}

/*	if(PLAYER2.score == PLAYER1.score && PLAYER1.Nserve < PLAYER2.Nserve)
	{
		GAME.SERVER = PLAYER_1;
	}

	if(PLAYER2.score == PLAYER1.score  && PLAYER2.Nserve < PLAYER1.Nserve)
	{
		GAME.SERVER = PLAYER_2;
	}
*/

	if(GAME.SERVER == PLAYER_1)
	{
		PLAYER1.Nserve ++;
	}

	if(GAME.SERVER == PLAYER_2)
	{
		PLAYER2.Nserve ++;
	}


	if(PLAYER1.score >= 100) // would laugh if this happen.
	{
		PLAYER1.score = 0;
		PLAYER2.score = 0;
		PLAYER1.Nserve = 0;
		PLAYER2.Nserve = 0;
		GAME.WINNER = PLAYER_1;
		GAME.WINNER_SERVED = PLAYER1.Nserve;
		GAME.MESSAGE =  "STOP F*C#@N% WITH ME";
		GAME.STATUS = stop;
		GAME.TOGGLE_WINNER = 1;
		//UPDATE_CURRENT_WINNER();

#ifdef DEBUG_GAME

		DISPLAY_SCORE();
		 USART_puts(USART3, "PLAYER1 MESSAGE -> ");
		USART_puts(USART3, PLAYER1.message);
#endif

	}

	if(PLAYER2.score >= 100) // would laugh if this happen.
	{
		PLAYER1.score = 0;
		PLAYER2.score = 0;
		PLAYER1.Nserve = 0;
		PLAYER2.Nserve = 0;
		GAME.WINNER = PLAYER_2;
		GAME.WINNER_SERVED = PLAYER2.Nserve;
		GAME.MESSAGE = "STOP F*C#@N% WITH ME";
		GAME.STATUS = stop;
		GAME.TOGGLE_WINNER = 1;
		//UPDATE_CURRENT_WINNER();

#ifdef DEBUG_GAME
		DISPLAY_SCORE();
		USART_puts(USART3, "PLAYER2 MESSAGE -> ");
		USART_puts(USART3, PLAYER2.message);
#endif
	}


#ifdef DEBUG_GAME
	eol(USART3);
	USART_puts(USART3, "GAME SERVER ->  ");
	DISPLAY_SERVER(GAME.SERVER);

/*	eol(USART3);
	USART_puts(USART3, "GAME SERVER SERVER2 ->  ");
	DISPLAY_SERVER(server2);
*/ //not needed

	eol(USART3);
	USART_puts(USART3, "GAME POINTS -> ");
	DISPLAY_INT2(USART3, GAME.POINTS, 10, 0);
	eol(USART3);

	USART_puts(USART3, "PLAYER 1 SCORE -> ");
	DISPLAY_INT2(USART3, PLAYER1.score, 10, 0);
	eol(USART3);

	USART_puts(USART3, "PLAYER 2 SCORE -> ");
	DISPLAY_INT2(USART3, PLAYER2.score, 10, 0);
	eol(USART3);

#endif


}

void UPDATE_CURRENT_WINNER(void)
{

		  USART_puts(USART3, GAME.WINNER_NAME);
		  char src[50], dest [50];
		  strcpy(src,GAME.WINNER_NAME);
		  strcpy(dest, "CURRENT WINNER : ");

		  USART_puts(USART3, src);
		  USART_puts(USART3, dest);
		  strncat(dest, src, strlen(src));
		  USART_puts(USART3, dest);
		  GAME.DISPLAY_WINNER = dest;
		  GAME.TOGGLE_WINNER = 0;
}

void DISPLAY_SCORE(void)
{
	if(GAME.STATUS == stop)
	{
		eol(USART3);
		USART_puts(USART3, "GAME WINNER -> ");
		DISPLAY_PLAYER(GAME.WINNER);
		//DISPLAY_INT2(USART3, GAME.WINNER, 10, 0);

		eol(USART3);
		USART_puts(USART3, "GAME WINNER SERVED -> ");
		DISPLAY_INT2(USART3, GAME.WINNER_SERVED, 10, 0);
	}
}

void INIT_PLAYERS(unsigned char PLAYERS)
{
	if(PLAYERS == PLAYER_1)
	{
		//PLAYER1.message = PLAYER1.message;
		//PLAYER1.name = PLAYER1.name;
		strcpy(PLAYER1.message , PLAYER1.message);
		strcpy(PLAYER1.name, PLAYER1.name);

		PLAYER1.score = 0;
		PLAYER1.Nserve = 0;
	}

	if(PLAYERS == PLAYER_2)
	{
		//PLAYER2.message = PLAYER2.message;
		//PLAYER2.name = PLAYER2.name;
		strcpy(PLAYER2.message , PLAYER2.message);
		strcpy(PLAYER2.name, PLAYER2.name);

		PLAYER2.score = 0;
		PLAYER2.Nserve = 0;
	}

}

void EE_MAP(void)
{
char TEMP_DATA3[I2C_EEPROM_PAGE];
uint16_t ee_map;
	for(ee_map = 40; ee_map <= 400; ee_map+=20)
	{

		read_from_eeprom(TEMP_DATA3, ee_map);
		USART_puts(USART3, TEMP_DATA3);
		eol(USART3);

	}
}



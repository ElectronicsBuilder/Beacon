
#include "GAME.h"
#include "log.h"
#include <stdio.h>
#include "Ez_USART.h"
#include "RANDOM_NUMBER.h"
#include "EEPROM.h"
#include "eeprom_24lc.h"

/* Global variables declared in GAME.h (defined here once) */
char ALL_TIME_SC[11][11][2];
GAMERS PLAYER1, PLAYER2, GAMER0, GAMER1, GAMER2, GAMER3,
GAMER4, GAMER5, GAMER6, GAMER7, GAMER8, GAMER9, GAMER10, GAMER_ALL[11];   /* 11 players, id 0..10 */
RECEIVER COMM;
STATUS GAME;

extern uint16_t id;   /* defined in TIM_CTRL.c */
#define PLAYER_DATA_START_ADDR 0x0000

uint16_t serv;
unsigned char CURRENT_SERVER[] = { no_server, PLAYER_1, PLAYER_2};

//#define P_LOOKUP
//#define SIMULATE_EE


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

	return GAMERX;

#endif

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
		/* EEPROM already initialised: nothing to write */
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





/* Print a player's record (name, message, wins and points against each
 * other player). Used by the console "game stats" command. */
void DISPLAY_PLAYER_INFO(GAMERS GAMERX)
{
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

}

void UPDATE_TEST_GAMER(GAMERS GAMERX, uint16_t id)
{

	GAMERX.id = id;
	GAMERX  = RETURN_SAVE_LOC(id, GAMERX);
	GAMERX = TEST_READ_DATA2_SCORE(id, GAMERX);

	GAMER_ALL[id] = GAMERX;
}


void SHIFT_PLAYERS(GAMERS GAMERX, uint16_t id, CONSOLE_STAT_T x)
{
	if(x == PLAYER_1)
	{
		PLAYER1 = GAMER_ALL[id]; //update player 1 info
		LOG_I("Player 1: %s", PLAYER1.name);
		return;
	}

	else if(x == PLAYER_2)
	{
		PLAYER2 = GAMER_ALL[id]; //update player 2 info
		LOG_I("Player 2: %s", PLAYER2.name);
		return;
	}


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





GAMERS UPDATE_RETURN_SCORE(GAMERS GAMERX, GAMERS GAMERY)
{
	uint8_t score_track;

	   score_track = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.PD_loc + GAMERY.id));
	   score_track ++;
	   Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.PD_loc + GAMERY.id), score_track);


	   GAMERX.PLAYER_DATA[GAMERY.id] = Eeprom24_ReadByte(EE24_ADDR_0, (GAMERX.PD_loc + GAMERY.id));


	  // TEST_GAMERS2(GAMER_ALL[GAMERX.id], GAMERX.id);
	  // TEST_GAMERS2(GAMER_ALL[GAMERY.id], GAMERY.id);

	   //GAMERX.PLAYER_DATA[GAMERY.id] = t_data;

	   return GAMERX;

}

void DISPLAY_CURRENT_SCORE(GAMERS GAMERX, GAMERS GAMERY)
{



  if(GAMERX.SAVED_SCORE_DATA[GAMERY.id] < ALL_TIME_SC[GAMERX.id][GAMERY.id][0])
  {
	  GAMERX.SAVED_SCORE_DATA[GAMERY.id] = ALL_TIME_SC[GAMERX.id][GAMERY.id][0];

	  Eeprom24_WriteByte(EE24_ADDR_0,(GAMERX.score_data_loc + GAMERY.id), GAMERX.SAVED_SCORE_DATA[GAMERY.id]);


  }

  if(GAMERY.SAVED_SCORE_DATA[GAMERX.id] < ALL_TIME_SC[GAMERX.id][GAMERY.id][1])
  {
  	  GAMERY.SAVED_SCORE_DATA[GAMERX.id] = ALL_TIME_SC[GAMERX.id][GAMERY.id][1];

  	  Eeprom24_WriteByte(EE24_ADDR_0,(GAMERY.score_data_loc + GAMERX.id), GAMERY.SAVED_SCORE_DATA[GAMERX.id]);

  }



  //--------------new stuff--------


  GAMERX = UPDATE_RETURN_SCORE(GAMERX, GAMERY);



  UPDATE_TEST_GAMER(GAMER_ALL[GAMERX.id], GAMERX.id);
  UPDATE_TEST_GAMER(GAMER_ALL[GAMERY.id], GAMERY.id);

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
		LOG_I("Game over: %s beat %s %d:%d", PLAYER1.name, PLAYER2.name,
		      ALL_TIME_SC[PLAYER1.id][PLAYER2.id][0], ALL_TIME_SC[PLAYER1.id][PLAYER2.id][1]);
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
				LOG_I("Game over: %s beat %s %d:%d", PLAYER2.name, PLAYER1.name,
				      ALL_TIME_SC[PLAYER2.id][PLAYER1.id][0], ALL_TIME_SC[PLAYER2.id][PLAYER1.id][1]);
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

	}




}







#ifndef _GAME_H
#define _GAME_H

#include "stm32f4xx.h"
#include "app_main.h"
#include "usart.h"
#include "app_hal.h"
#include "app_hal.h"
#include "app_hal.h"
#include "app_hal.h"

#include "EEPROM.h"

#define DEBUG_DISPLAY_PLAYER


#define I2C_EEPROM_PAGE   (32)       // EEPROM memory page size

typedef enum CONSOLE_STAT
{
	failed = 0,
	init_success = 1,
	time_success = 2,
	PLAYER_1 = 3,
	PLAYER_2 = 4,
	start = 5,
	stop = 6,
	idle = 7,
	saved = 8,
	unsaved =9,
	no_server = 10,
	display_random_server = 11,
	update_current_winner = 12,
	select_player1_mode = 13,
	select_player2_mode = 14,
	display_error_msg = 15,
	display_score_info = 16,
	sleep_display_enable = 17,
	sleep_display_disable = 18

}CONSOLE_STAT_T;

typedef struct
{
	uint16_t buffer_size;
	char STX;
	char CODE1;
	char CODE2;
	unsigned char* DATA;
	unsigned char* COMMAND;
	char ETX;
}RECEIVER;


typedef struct
{
	unsigned char* NAME;
	uint16_t SCORE;
	char* MESSAGE;
	uint16_t nSERVED;

}PLAYERS;

typedef struct
{
	uint16_t  START ;
	uint16_t  STOP;
	uint16_t TYPE;
	char* MESSAGE;
	char* ERROR_MSG;
	uint16_t initial_players_name;
	uint16_t SERVER;
	uint16_t WINNER;
	uint16_t POINTS;
	uint16_t  WINNER_SERVED;
	uint16_t STATUS;
	uint16_t COMMAND;
	char* WINNER_NAME;
	char* LOSER_NAME;
	char*DISPLAY_WINNER;
	uint16_t TOGGLE_WINNER;


}STATUS;


 char ALL_TIME_SC[10][10][2];


typedef struct
{
	uint16_t id;
	uint16_t current_score;
	uint16_t score;
	uint16_t AT_score;
	uint16_t Nserve;
	uint16_t name_loc;
	uint16_t message_loc;
	uint16_t PD_loc; // temp Data location
	uint16_t score_data_loc;

   char* name_M;
	char* message_M;

	char name[I2C_EEPROM_PAGE];
	char message[I2C_EEPROM_PAGE];

	char PLAYER_DATA[I2C_EEPROM_PAGE];
	char SCORE_KEEPER[1][1][2];
	char TEMP_DATA[I2C_EEPROM_PAGE];

	char SCORE_DATA[I2C_EEPROM_PAGE][1];

	char SAVED_SCORE_DATA[I2C_EEPROM_PAGE];

}GAMERS;



GAMERS PLAYER1, PLAYER2, GAMER0, GAMER1, GAMER2, GAMER3,
GAMER4, GAMER5, GAMER6, GAMER7, GAMER8, GAMER9, GAMER10, HOLD1, HOLD2,GAMER_ALL[10];


RECEIVER COMM;

STATUS GAME;


void  RETURN_RANDOM_SERVER(void);
uint16_t RETURN_GAME_SERVER(void);
GAMERS PORT_GAMER_FROM_ID(uint16_t id);
GAMERS UPDATE_GAMER_DATA(GAMERS GAMERX, GAMERS GAMERY);
GAMERS RETURN_SAVE_LOC(uint16_t id, GAMERS GAMERX);
GAMERS SAVING_GAMER_DATA(GAMERS GAMERX);
GAMERS SAVING_GAMER_DATA2(uint16_t id, GAMERS GAMERX);
GAMERS text_player_data(GAMERS GAMERX, uint16_t id);
void TEST_READ_DATA(uint16_t id);
GAMERS TEST_READ_DATA2(uint16_t id, GAMERS GAMERX);
void DISPLAY_PLAYER_INFO(GAMERS GAMERX);
void TEST_GAMERS2(GAMERS GAMERX, uint16_t id);
void TEST_GAMERS3(uint16_t id);
void SHIFT_PLAYERS(GAMERS GAMERX, uint16_t id, CONSOLE_STAT_T x);
void TEST_GAMERS(void);
void EXTR_PLAYER_DATA(void);
void DISPLAY_SERVER(uint16_t SERVER);
void DISPLAY_PLAYER(uint16_t NAME);
void GAME_TYPE_1(void);
void DISPLAY_SCORE(void);
void INIT_PLAYERS(unsigned char PLAYERS);
void UPDATE_CURRENT_WINNER(void);


void DISPLAY_CURRENT_SCORE(GAMERS GAMERX, GAMERS GAMERY);
void RESET_SCORE(void);


GAMERS UPDATE_RETURN_SCORE(GAMERS GAMERX, GAMERS GAMERY);
GAMERS TEST_READ_DATA2_SCORE(uint16_t id, GAMERS GAMERX);
void UPDATE_TEST_GAMER(GAMERS GAMERX, uint16_t id);


#endif /*_GAME_H*/

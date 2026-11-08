

#include "RANDOM_NUMBER.h"
#include "GAME.h"


char PLAYER_RANDOM[] = {1 ,2 ,1, 2, 1, 2, 1, 2, 1, 2};


// RANDOM NUMBER


uint32_t RANDOM_NUMBER;
char RANDOM_NUMBER_BUFFER[10];

void RNG_Init(void) {
	/* RNG is initialised by CubeMX (MX_RNG_Init). */
}

uint32_t RNG_Get(void) {
	uint32_t value = 0;
	HAL_RNG_GenerateRandomNumber(&hrng, &value);
	return value;
}


uint16_t GET_RANDOM(void)
{
	 RANDOM_NUMBER = RNG_Get();

	 itoa (RANDOM_NUMBER, buffer, 10);
	 for(x = 0; x<= 10; x++)
	 {
		 RANDOM_NUMBER_BUFFER[x] = buffer[x] - 0x30;
	 }

	  return PLAYER_RANDOM[RANDOM_NUMBER_BUFFER[5]];


}










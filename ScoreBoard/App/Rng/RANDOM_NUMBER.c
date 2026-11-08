

#include "RANDOM_NUMBER.h"
#include "GAME.h"


char PLAYER_RANDOM[] = {1 ,2 ,1, 2, 1, 2, 1, 2, 1, 2};


// RANDOM NUMBER


uint32_t RANDOM_NUMBER;

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

	 /* random first server: 1 or 2 (table lookup with a random digit) */
	 return PLAYER_RANDOM[RANDOM_NUMBER % 10u];


}










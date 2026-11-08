

#include "TIM_CTRL.h"
#include "Ez_USART.h"
#include "app_timers.h"
#include "GAME.h"
#include "matrix.h"
#include "FONT.h"
#include "rtc_clock.h"

#include "app_main.h"


//#define DEBUG_USB_HID_KEYBOARD

int32_t scroll1=0,scroll2=0;

uint16_t id, id2;


void APP_TIM2_Tick(void)   /* RTC clock refresh, 1 s */
{
	  if (1) /* update flag already cleared by HAL_TIM_IRQHandler */
	  {
		
	  //  timer_count2 += 1;
	   }
if(GAME.STATUS == start)
{
	  RTC_TimeShow();

		itoa(SEC_H, D_SEC, 16);
		itoa(MIN_H, D_MIN, 16);
		itoa(HOUR_H,D_HOUR, 16);

	 	 calculateLEDString("TM",0,24,Font8x8);

	 	 CHANGE_FONT(GREEN);

	     if(MIN_H < 9)
	     {
	    	 calculateLEDString("  ",18,24,Font8x8);
	    	 calculateLEDString(D_MIN,18,24,Font8x8);
	     }
	     if(MIN_H > 10)
	     {
	    	 calculateLEDString(D_MIN,18,24,Font8x8);
	     }

	 	 CHANGE_FONT(BLUE);

	 	 calculateLEDString(":",33,24,Font8x8);

	 	 CHANGE_FONT(RED);

	     if(SEC_H < 9)
	     {
	     	calculateLEDString("  ",40,24,Font8x8);
	     	calculateLEDString(D_SEC,40,24,Font8x8);
	     }

	     if(SEC_H > 10)
	     {
	    	 calculateLEDString(D_SEC,40,24,Font8x8);
	     }


	 	 CHANGE_FONT(BLUE);
	}

}


char* strncat2(char *dest, const char *src, size_t n)
{
    size_t dest_len = strlen(dest);
    size_t i;

   for (i = 0 ; i < n && src[i] != '\0' ; i++)
        dest[dest_len + i] = src[i];
    dest[dest_len + i] = '\0';

   return dest;
}




void APP_TIM3_Tick(void)
{

  if (1) /* update flag already cleared by HAL_TIM_IRQHandler */
  {
	
   }

  	  if(GAME.STATUS != stop)
  	  {
  		 GAME.MESSAGE = "NO MSG YET";
  	  }

  	  if(GAME.STATUS == display_error_msg)
  	  {
  		GAME.MESSAGE = GAME.ERROR_MSG;
  	  }
  	  if( GAME.STATUS == stop)
  	  	  {

  	  	//  USART_puts(USART3, GAME.WINNER_NAME); eol(USART3);
  		  char src[50], dest [50];
  		  strcpy(src,GAME.WINNER_NAME);
  		  strcpy(dest, "CURRENT WINNER : ");

  		 // USART_puts(USART3, src);			eol(USART3);
  		 // USART_puts(USART3, dest);			eol(USART3);
  		  strncat(dest, src, strlen(src));
  		 // SART_puts(USART3, dest);			eol(USART3);






  		  char src2[50], dest2[50];

  		strcpy(src2,GAME.LOSER_NAME);
  		strcpy(dest2, " & LOSER : ");

  		strncat(dest2, src2, strlen(src2));

  		strncat(dest, dest2, strlen(dest2));


  		 GAME.DISPLAY_WINNER = dest;






  		// USART_puts(USART3, GAME.DISPLAY_WINNER);
  		//calculateScrollText(GAME.DISPLAY_WINNER,Font8x8,28,0,&scroll1);
  		  GAME.TOGGLE_WINNER = 0;
  	  }

  	  else {
  		GAME.DISPLAY_WINNER = "CURRENT WINNER: ";
	}

  	  	  calculateScrollText(GAME.DISPLAY_WINNER,Font8x8,28,0,&scroll1);
  	  	  calculateScrollText(GAME.MESSAGE,Font8x8,15,16,&scroll2);




}


void APP_TIM4_Tick(void)
{

  if (1) /* update flag already cleared by HAL_TIM_IRQHandler */
  {
	
  }


  if(GAME.STATUS != start)
  {

	  PLAYER1.AT_score = PLAYER1.SAVED_SCORE_DATA[PLAYER2.id];
	  PLAYER2.AT_score = PLAYER2.SAVED_SCORE_DATA[PLAYER1.id];

  	 	 	 calculateLEDString("HS",0,24,Font8x8);

  		 	 CHANGE_FONT(GREEN);

  		 	 tostring(number5, PLAYER1.AT_score);
  		 	 tostring(number6, PLAYER2.AT_score);


  			 if(PLAYER1.AT_score < 9)
  				     {
  				    	 calculateLEDString("  ",18,24,Font8x8);
  				    	 calculateLEDString(number5,18,24,Font8x8);
  				     }
  				     if(PLAYER1.AT_score > 10)
  				     {
  				    	 calculateLEDString(number5,18,24,Font8x8);
  				     }

  				 	 CHANGE_FONT(BLUE);

  				 	 calculateLEDString(":",33,24,Font8x8);

  				 	 CHANGE_FONT(RED);

  				     if(PLAYER2.AT_score < 9)
  				     {
  				     	calculateLEDString("  ",40,24,Font8x8);
  				     	calculateLEDString(number6,40,24,Font8x8);
  				     }

  				     if(PLAYER2.AT_score > 10)
  				     {
  				    	 calculateLEDString(number6,40,24,Font8x8);
  				     }


  				 	 CHANGE_FONT(BLUE);


  	}

	  if(GAME.STATUS == idle || GAME.STATUS == stop)
	  {
		calculateLEDString("  SCORE  BOARD    ",0,8,Font8x8);
		GAME.SERVER = no_server;
		STOP_TIMER(RTC_TIM_TYPE);
		/*
		PLAYER1.score = 0;
		PLAYER2.score = 0;
		PLAYER1.Nserve = 0;
		PLAYER2.Nserve = 0;




		*/
		//RESET_SCORE();

		//GAME.STATUS = idle; // need to go back to idle mode
	  }



	  if(GAME.STATUS == select_player1_mode)
	  {
		  CHANGE_FONT(NAME_SELECT_P1_COLOR);
		  calculateLEDString( "         ", 0, 8, Font8x8);
		  calculateLEDString( PLAYER1.name, 0, 8, Font8x8);

		  PLAYER1.score = PLAYER1.PLAYER_DATA[PLAYER2.id];
		  PLAYER2.score = PLAYER2.PLAYER_DATA[PLAYER1.id];

		//	tostring(numberscore1, PLAYER2.score);
		//	 calculateLEDString(numberscore1,18+WIDTH/2,24,Font8x8);

			// DISPLAY_INT2(USART3, numberscore1,10, 1);
	  }

	  if(GAME.STATUS == select_player2_mode)
	  {
		  CHANGE_FONT(NAME_SELECT_P2_COLOR);
		  calculateLEDString( "         ", 56, 8, Font8x8);
		  calculateLEDString(PLAYER2.name, 72, 8, Font8x8);

		  PLAYER1.score = PLAYER1.PLAYER_DATA[PLAYER2.id];
		  PLAYER2.score = PLAYER2.PLAYER_DATA[PLAYER1.id];


		//	 tostring(numberscore2, PLAYER1.score);
		 //   calculateLEDString(numberscore2,40+WIDTH/2,24,Font8x8);
		 //    DISPLAY_INT2(USART3, numberscore2,10, 1);

	  }




	tostring(number3, PLAYER2.score);
	tostring(number4, PLAYER1.score);


    calculateLEDString("SC",0+WIDTH/2,24,Font8x8);
    CHANGE_FONT(RED);

    if(PLAYER1.score < 10)
    {
    	calculateLEDString("  ",18+WIDTH/2,24,Font8x8);
        calculateLEDString(number4,26+WIDTH/2,24,Font8x8);
    }
    if(PLAYER1.score > 9)
    {
    	 calculateLEDString(number4,18+WIDTH/2,24,Font8x8);
    }

    CHANGE_FONT(BLUE);

    calculateLEDString(":",33+WIDTH/2,24,Font8x8);

    CHANGE_FONT(GREEN);



    if(PLAYER2.score < 10)
    {
    	calculateLEDString("  ",40+WIDTH/2,24,Font8x8);
    	calculateLEDString(number3,40+WIDTH/2,24,Font8x8);
    }

    if(PLAYER2.score > 9)
    {
        calculateLEDString(number3,40+WIDTH/2,24,Font8x8);
    }



    CHANGE_FONT(BLUE);



  	  if(GAME.STATUS == start)
  	  {
  		  if(GAME.COMMAND == display_random_server)
  		  {
  			uint16_t serv2 = RETURN_GAME_SERVER();
  			unsigned char CURRENT_SERVER2[] = { no_server, PLAYER_1, PLAYER_2};
  			GAME.SERVER = CURRENT_SERVER2[serv2];
  			GAME.COMMAND = 0;
  		  }


  		  	 CHANGE_FONT(WHITE);
  		  	 calculateLEDString("VS",56,8,Font8x8 );
  		  	 if(GAME.initial_players_name == 1)
  		  	 {
  		  		 if(GAME.SERVER == PLAYER_1)
  		  		 {
  		  			 calculateLEDString( "       ", 0, 8, Font8x8);
  		  			 CHANGE_FONT(NAME_SERVER_COLOR);
  		  		  	 calculateLEDString( PLAYER1.name, 0, 8, Font8x8);
  		  		 }

  		  		 else
  		  		 {
  		  		 calculateLEDString( "       ", 0, 8, Font8x8);
  		  			 CHANGE_FONT(NAME_RECEIVER_COLOR);
  		  		  	 calculateLEDString( PLAYER1.name, 0, 8, Font8x8);
				 }

  		  		 if(GAME.SERVER == PLAYER_2)
  		  		 {
  		  			 calculateLEDString( "       ", 72, 8, Font8x8);
  	  	  		     CHANGE_FONT(NAME_SERVER_COLOR);
  	  	  		     calculateLEDString(PLAYER2.name, 72, 8, Font8x8);
  		  		 }
  		  		 else
  		  		 {
  		  			 calculateLEDString( "       ", 72, 8, Font8x8);
  	  	  		     CHANGE_FONT(NAME_RECEIVER_COLOR);
  	  	  		     calculateLEDString(PLAYER2.name, 72, 8, Font8x8);
  		  		 }

  		  	 }

  		  	 CHANGE_FONT(BLUE);

  	  }

  	 if(SLEEP_STAT ==sleep_display_enable )
  	  {
  		  SCREEN_SLEEP++;
  		//  DISPLAY_INT2(USART3, SCREEN_SLEEP, 10, 1);
  	  }



  	 if(SCREEN_SLEEP == 2 )
  	 {
  		 SCREEN_SLEEP_MIN++;
  		// DISPLAY_INT2(USART3, SCREEN_SLEEP, 10, 1);
  		 SCREEN_SLEEP = 0;
  	 }

  	if(SCREEN_SLEEP_MIN == 2)
  	{
  		SCREEN_SLEEP_MIN = 0;
  		DISPLAY_SLEEP();
  	}

}







void APP_TIM5_Tick(void) // usb device not supported handler
{

  if (1) /* update flag already cleared by HAL_TIM_IRQHandler */
  {
	
  }



timer_count++;
if(timer_count >= 2)
{
	  calculateLEDdata();
}


}



void DISPLAY_SLEEP(void)
{
	STOP_UPDATE_DISPLAY();
    calculateClearImage();
    updateLEDData();
}

void KEYBOARD_CONTROL(uint8_t data)
{
	/* LED matrix brightness: '+' brighter, '-' dimmer (not used by the game) */
	if(data == '+' || data == '=')
	{
		Matrix_SetBrightness(Matrix_GetBrightness() + MATRIX_BRIGHTNESS_STEP);
	}
	if(data == '-' || data == '_')
	{
		uint8_t b = Matrix_GetBrightness();
		Matrix_SetBrightness(b > MATRIX_BRIGHTNESS_STEP ? b - MATRIX_BRIGHTNESS_STEP : MATRIX_BRIGHTNESS_MIN);
	}
	if(data == '@')
	{
		SLEEP_STAT = sleep_display_disable;
		RESUME_UPDATE_DISPLAY();
	}
	if(data == '^')
	{
		SLEEP_STAT = sleep_display_enable;
	}
	if(data == '/')
		         {
		           STOP_UPDATE();
		         }
		         if(data == '|')
		         {
		          RESUME_UPDATE();
		         }

		         if(GAME.STATUS == start)
		         {
		                 if(data == 'a')
		               {
		                 PLAYER1.score++;
		                 GAME.COMMAND = 1;
		               }

		               if(data == 'z')
		               {
		            	   if(PLAYER1.score <= 0)
		                        	   {
		                        		   PLAYER1.score = 0;
		                        	   }
		            	   else

		                 PLAYER1.score--;
		                 GAME.COMMAND = 1;
		               }

		               if(data == 'k')
		               {
		                 PLAYER2.score++;
		                 GAME.COMMAND = 1;
		               }
		               if(data == 'm')
		               {
		            	   if(PLAYER2.score <= 0)
		                        	   {
		                        		   PLAYER2.score = 0;
		                        	   }
		            	   else

		                 PLAYER2.score--;
		                 GAME.COMMAND = 1;
		               }

		               else
		               {
		            	   GAME.COMMAND = 0;
		               }



		         }

		         if(GAME.STATUS != start)
		         	         {
		        	 RESET_SCORE();


		         	        	 if(data == 'a')
		         		        	 {
		         	        		 	// GAME.STATUS = select_player1_mode;
		         		        		 SHIFT_PLAYERS(GAMER_ALL[id], id, PLAYER_1);
		         		        		 GAME.STATUS = select_player1_mode;
		         		        		 id++;
		         		        		 if(id > 10)
		         		        		 {
		         		        			 id = 0;
		         		        		 }
		         		        	 }

		         	        	 else if(data == 'k')
		         		        	 {
		         	        		 	 //GAME.STATUS = select_player2_mode;
		         		        		 SHIFT_PLAYERS(GAMER_ALL[id2], id2, PLAYER_2);
		         		        		 GAME.STATUS = select_player2_mode;
		         		        		 id2++;
		         		        		 if(id2 > 10)
		         		        		 {
		         		        			 id2 = 0;
		         		        		 }
		         		        	 }

/*

		         	        	 else if(data == 'z')
		         		        	 {
		         	        		 	 GAME.STATUS = select_player1_mode;
		         		        		 SHIFT_PLAYERS(GAMER_ALL[id], id, PLAYER_1);
		         		        		 id--;
		         		        		 if(id < 1)
		         		        		 {
		         		        			 id = 10;
		         		        		 }
		         		        	 }

		         	        	 else if(data == 'm')
		         		        	 {
		         	        		 	 GAME.STATUS = select_player2_mode;
		         		        		 SHIFT_PLAYERS(GAMER_ALL[id2], id2, PLAYER_2);
		         		        		 id2--;
		         		        		 if(id2 < 1)
		         		        		 {
		         		        			 id2 = 10;
		         		        		 }
		         		        	 }

*/
		         	         }


		         	               if(data == 's')
		         	               {
		         	#ifdef DEBUG_GAME_START_CMD

		         	            	   USART_puts(USART3, "PLAYER1 MESSAGE -> ");
		         	            	   USART_puts(USART3, PLAYER1.message);
		         	            	   eol(USART3);
		         	            	   USART_puts(USART3, "PLAYER2 MESSAGE -> ");
		         	            	   USART_puts(USART3, PLAYER2.message);
		         	#endif

		         	            	 if(PLAYER1.id == PLAYER2.id)
		         	            	 {
		         	            		 GAME.STATUS = display_error_msg;
		         	            		 GAME.ERROR_MSG = "CHECK PLAYERS ID";
		         	            	 }
		         	            	 else
		         	            	 {
		         		            	 GAME.START = 1;
		         		                 GAME.STATUS = start;
		         		                 GAME.COMMAND = display_random_server;
		         		                GAME.POINTS = 0;

		     			             	RTC_RESET_TIME();
		     			             	//RESET_SCORE();
		         	            	 }



		         	               }

		         	         //      COMMAND_RECEIVER(data);

		         	               if(data == 'a' || data == 'k')
		         	               {
		         	            	   GAME.POINTS ++;
		         	            	   GAME_TYPE_1();
		         	               }

		         	               if(data == 'z' || data == 'm')
		         	               {
		         	            	   GAME.POINTS --;
		         	            	 //  GAME_TYPE_1();
		         	               }
}

void APP_TIM7_Tick(void) // usb device not supported handler
{

  if (1) /* update flag already cleared by HAL_TIM_IRQHandler */
  {
	
  }

  UPDATE_COUNTER++;
  if(UPDATE_COUNTER >= 4)
  {
	  UPDATE_COUNTER = 0;

	  RESUME_DATA_UPDATE();
  }

  if(UPDATE_COUNTER <= 4)
  {

	  STOP_DATA_UPDATE();
  }

  uint8_t data = POLL_USB();

#ifdef DEBUG_USB_HID_KEYBOARD
if(data != 0)
{
	  USART_putc(USART3, data);
}
#endif

  KEYBOARD_CONTROL(data);

}



void STOP_DATA_UPDATE(void)
{
	STOP_TIMER(TIM5);
}


void RESUME_DATA_UPDATE(void)
{
	  INIT_TIMER_CONFIG(RCC_APB1Periph_TIM5, TIM5_IRQn, TIM5, 10);
}
void STOP_UPDATE(void)
{
	//STOP_TIMER(TIM2);
	STOP_TIMER(TIM3);
	STOP_TIMER(TIM4);
	//STOP_TIMER(TIM5);

}

void STOP_UPDATE_DISPLAY(void)
{
	STOP_TIMER(RTC_TIM_TYPE);
	STOP_TIMER(TIM3);
	STOP_TIMER(TIM4);
}

void RESUME_UPDATE_DISPLAY(void)
{
    INIT_TIMER_CONFIG(RCC_APB1Periph_TIM3, TIM3_IRQn, TIM3, 300); // ON THE FLY MESSAGE UPDATE
    INIT_TIMER_CONFIG(RCC_APB1Periph_TIM4, TIM4_IRQn, TIM4, 1000);
    INIT_TIMER_CONFIG(RTC_TIMER_PERIPH, RTC_TIMER_IRQ, RTC_TIM_TYPE, RTC_UPDATE_TIME);
}


void RESUME_UPDATE(void)
{
   // INIT_TIMER_CONFIG(RCC_APB1Periph_TIM2, TIM2_IRQn, TIM2, 10000);
    INIT_TIMER_CONFIG(RCC_APB1Periph_TIM3, TIM3_IRQn, TIM3, 300); // ON THE FLY MESSAGE UPDATE
    INIT_TIMER_CONFIG(RCC_APB1Periph_TIM4, TIM4_IRQn, TIM4, 1000);
  //  INIT_TIMER_CONFIG(RCC_APB1Periph_TIM5, TIM5_IRQn, TIM5, 100);

}



/*
 * Called from HAL_TIM_PeriodElapsedCallback() (Core/Src/main.c) for every
 * timer update interrupt. Dispatches to the original timer handlers.
 */
void APP_TIM_PeriodElapsed(TIM_HandleTypeDef *htim)
{
  if      (htim->Instance == TIM2) APP_TIM2_Tick();
  else if (htim->Instance == TIM3) APP_TIM3_Tick();
  else if (htim->Instance == TIM4) APP_TIM4_Tick();
  else if (htim->Instance == TIM5) APP_TIM5_Tick();
  else if (htim->Instance == TIM7) APP_TIM7_Tick();
}



#include "log.h"
#include <stdio.h>
#include "TIM_CTRL.h"
#include "layout.h"
#include "Ez_USART.h"
#include "app_timers.h"
#include "GAME.h"
#include "matrix.h"
#include "FONT.h"
#include "game_clock.h"

#include "app_main.h"

/* Global variables declared in TIM_CTRL.h (defined here once) */
uint16_t SCREEN_SLEEP, SCREEN_SLEEP_MIN, SLEEP_STAT;




int32_t scroll1=0,scroll2=0;

uint16_t id, id2;   /* player selection indexes (also used by Comm.c and GAME.c) */


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

	 	 calculateLEDString("TM",0,LAYOUT_LINE_Y(3),Font8x8);

	 	 CHANGE_FONT(GREEN);

	     if(MIN_H < 9)
	     {
	    	 calculateLEDString("  ",18,LAYOUT_LINE_Y(3),Font8x8);
	    	 calculateLEDString(D_MIN,18,LAYOUT_LINE_Y(3),Font8x8);
	     }
	     if(MIN_H > 10)
	     {
	    	 calculateLEDString(D_MIN,18,LAYOUT_LINE_Y(3),Font8x8);
	     }

	 	 CHANGE_FONT(BLUE);

	 	 calculateLEDString(":",33,LAYOUT_LINE_Y(3),Font8x8);

	 	 CHANGE_FONT(RED);

	     if(SEC_H < 9)
	     {
	     	calculateLEDString("  ",40,LAYOUT_LINE_Y(3),Font8x8);
	     	calculateLEDString(D_SEC,40,LAYOUT_LINE_Y(3),Font8x8);
	     }

	     if(SEC_H > 10)
	     {
	    	 calculateLEDString(D_SEC,40,LAYOUT_LINE_Y(3),Font8x8);
	     }


	 	 CHANGE_FONT(BLUE);
	}

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
  		  /* static: the scroll keeps using this text after the handler returns */
  		  static char winner_text[80];
  		  snprintf(winner_text, sizeof winner_text, "CURRENT WINNER : %s & LOSER : %s",
  		           GAME.WINNER_NAME, GAME.LOSER_NAME);
  		  GAME.DISPLAY_WINNER = winner_text;






  		// USART_puts(USART3, GAME.DISPLAY_WINNER);
  		//calculateScrollText(GAME.DISPLAY_WINNER,Font8x8,28,LAYOUT_LINE_Y(0),&scroll1);
  		  GAME.TOGGLE_WINNER = 0;
  	  }

  	  else {
  		GAME.DISPLAY_WINNER = "CURRENT WINNER: ";
	}

  	  	  calculateScrollText(GAME.DISPLAY_WINNER,Font8x8,28,LAYOUT_LINE_Y(0),&scroll1);
  	  	  calculateScrollText(GAME.MESSAGE,Font8x8,15,LAYOUT_LINE_Y(2),&scroll2);




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

  	 	 	 calculateLEDString("HS",0,LAYOUT_LINE_Y(3),Font8x8);

  		 	 CHANGE_FONT(GREEN);

  		 	 tostring(number5, PLAYER1.AT_score);
  		 	 tostring(number6, PLAYER2.AT_score);


  			 if(PLAYER1.AT_score < 9)
  				     {
  				    	 calculateLEDString("  ",18,LAYOUT_LINE_Y(3),Font8x8);
  				    	 calculateLEDString(number5,18,LAYOUT_LINE_Y(3),Font8x8);
  				     }
  				     if(PLAYER1.AT_score > 10)
  				     {
  				    	 calculateLEDString(number5,18,LAYOUT_LINE_Y(3),Font8x8);
  				     }

  				 	 CHANGE_FONT(BLUE);

  				 	 calculateLEDString(":",33,LAYOUT_LINE_Y(3),Font8x8);

  				 	 CHANGE_FONT(RED);

  				     if(PLAYER2.AT_score < 9)
  				     {
  				     	calculateLEDString("  ",40,LAYOUT_LINE_Y(3),Font8x8);
  				     	calculateLEDString(number6,40,LAYOUT_LINE_Y(3),Font8x8);
  				     }

  				     if(PLAYER2.AT_score > 10)
  				     {
  				    	 calculateLEDString(number6,40,LAYOUT_LINE_Y(3),Font8x8);
  				     }


  				 	 CHANGE_FONT(BLUE);


  	}

	  if(GAME.STATUS == idle || GAME.STATUS == stop)
	  {
		calculateLEDString("  SCORE  BOARD    ",0,LAYOUT_LINE_Y(1),Font8x8);
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
		  calculateLEDString( "         ", 0, LAYOUT_LINE_Y(1), Font8x8);
		  calculateLEDString( PLAYER1.name, 0, LAYOUT_LINE_Y(1), Font8x8);

		  PLAYER1.score = PLAYER1.PLAYER_DATA[PLAYER2.id];
		  PLAYER2.score = PLAYER2.PLAYER_DATA[PLAYER1.id];

		//	tostring(numberscore1, PLAYER2.score);
		//	 calculateLEDString(numberscore1,18+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);

			// DISPLAY_INT2(USART3, numberscore1,10, 1);
	  }

	  if(GAME.STATUS == select_player2_mode)
	  {
		  CHANGE_FONT(NAME_SELECT_P2_COLOR);
		  calculateLEDString( "         ", 56, LAYOUT_LINE_Y(1), Font8x8);
		  calculateLEDString(PLAYER2.name, 72, LAYOUT_LINE_Y(1), Font8x8);

		  PLAYER1.score = PLAYER1.PLAYER_DATA[PLAYER2.id];
		  PLAYER2.score = PLAYER2.PLAYER_DATA[PLAYER1.id];


		//	 tostring(numberscore2, PLAYER1.score);
		 //   calculateLEDString(numberscore2,40+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);
		 //    DISPLAY_INT2(USART3, numberscore2,10, 1);

	  }




	tostring(number3, PLAYER2.score);
	tostring(number4, PLAYER1.score);


    calculateLEDString("SC",0+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);
    CHANGE_FONT(RED);

    if(PLAYER1.score < 10)
    {
    	calculateLEDString("  ",18+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);
        calculateLEDString(number4,26+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);
    }
    if(PLAYER1.score > 9)
    {
    	 calculateLEDString(number4,18+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);
    }

    CHANGE_FONT(BLUE);

    calculateLEDString(":",33+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);

    CHANGE_FONT(GREEN);



    if(PLAYER2.score < 10)
    {
    	calculateLEDString("  ",40+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);
    	calculateLEDString(number3,40+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);
    }

    if(PLAYER2.score > 9)
    {
        calculateLEDString(number3,40+WIDTH/2,LAYOUT_LINE_Y(3),Font8x8);
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
  		  	 calculateLEDString("VS",56,LAYOUT_LINE_Y(1),Font8x8 );
  		  	 if(GAME.initial_players_name == 1)
  		  	 {
  		  		 if(GAME.SERVER == PLAYER_1)
  		  		 {
  		  			 calculateLEDString( "       ", 0, LAYOUT_LINE_Y(1), Font8x8);
  		  			 CHANGE_FONT(NAME_SERVER_COLOR);
  		  		  	 calculateLEDString( PLAYER1.name, 0, LAYOUT_LINE_Y(1), Font8x8);
  		  		 }

  		  		 else
  		  		 {
  		  		 calculateLEDString( "       ", 0, LAYOUT_LINE_Y(1), Font8x8);
  		  			 CHANGE_FONT(NAME_RECEIVER_COLOR);
  		  		  	 calculateLEDString( PLAYER1.name, 0, LAYOUT_LINE_Y(1), Font8x8);
				 }

  		  		 if(GAME.SERVER == PLAYER_2)
  		  		 {
  		  			 calculateLEDString( "       ", 72, LAYOUT_LINE_Y(1), Font8x8);
  	  	  		     CHANGE_FONT(NAME_SERVER_COLOR);
  	  	  		     calculateLEDString(PLAYER2.name, 72, LAYOUT_LINE_Y(1), Font8x8);
  		  		 }
  		  		 else
  		  		 {
  		  			 calculateLEDString( "       ", 72, LAYOUT_LINE_Y(1), Font8x8);
  	  	  		     CHANGE_FONT(NAME_RECEIVER_COLOR);
  	  	  		     calculateLEDString(PLAYER2.name, 72, LAYOUT_LINE_Y(1), Font8x8);
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







void DISPLAY_SLEEP(void)
{
	STOP_UPDATE_DISPLAY();
    calculateClearImage();
    updateLEDData();
}

void KEYBOARD_CONTROL(uint8_t data)
{
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

		         	            	 if(PLAYER1.id == PLAYER2.id)
		         	            	 {
		         	            		 GAME.STATUS = display_error_msg;
		         	            		 GAME.ERROR_MSG = "CHECK PLAYERS ID";
		         	            		 LOG_W("Cannot start: both sides are the same player");
		         	            	 }
		         	            	 else
		         	            	 {
		         		            	 GAME.START = 1;
		         		                 GAME.STATUS = start;
		         		                 LOG_I("Game started: %s vs %s", PLAYER1.name, PLAYER2.name);
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



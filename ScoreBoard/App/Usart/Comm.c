


#include "log.h"
#include "Comm.h"
#include "TIM_CTRL.h"
#include "matrix.h"
#include "GAME.h"

#include "Ez_USART.h"
#include "app_main.h"
#include "FONT.h"
#include "rtc_clock.h"

uint16_t id, id2;






/* Radio (HM-TRP) byte handler. Runs in the App task; bytes come from the
 * USART2 receive ring buffer (Usart/uart_rx.c). */
void APP_USART2_Byte(char data)
{
       {

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

	               if(data == 'b')
	               {
	            	   if(PLAYER1.score <= 0)
	                        	   {
	                        		   PLAYER1.score = 0;
	                        	   }
	            	   else

	                 PLAYER1.score--;
	                 GAME.COMMAND = 1;
	               }

	               if(data == 'c')
	               {
	                 PLAYER2.score++;
	                 GAME.COMMAND = 1;
	               }
	               if(data == 'd')
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
	        		 	 GAME.STATUS = select_player1_mode;
		        		 SHIFT_PLAYERS(GAMER_ALL[id], id, PLAYER_1);
		        		 id++;
		        		 if(id > 10)
		        		 {
		        			 id = 0;
		        		 }
		        	 }

	        	 if(data == 'c')
		        	 {
	        		 	 GAME.STATUS = select_player2_mode;
		        		 SHIFT_PLAYERS(GAMER_ALL[id2], id2, PLAYER_2);
		        		 id2++;
		        		 if(id2 > 10)
		        		 {
		        			 id2 = 0;
		        		 }
		        	 }

	             if(data == 'x')
	             {
	               PLAYER1 =  SAVING_GAMER_DATA2(PLAYER1.id, PLAYER1);
	               PLAYER2 =  SAVING_GAMER_DATA2(PLAYER2.id, PLAYER2);

	               DISPLAY_CURRENT_SCORE(PLAYER1, PLAYER2);   // DISPLAY PREVIOUS SCORE
	             }





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

			             	RTC_RESET_TIME();
			             	GAME.POINTS = 0;
			             	//RESET_SCORE();

	            	 }



	               }

	         //      COMMAND_RECEIVER(data);

	               if(data == 'a' || data == 'c')
	               {
	            	   GAME.POINTS ++;
	            	   GAME_TYPE_1();
	               }


 	               if(data == 'z' || data == 'm')
 	               {
 	            	   GAME.POINTS --;
 	            	  // GAME_TYPE_1();
 	               }


	         COMMAND_RECEIVER2(data);


	       }

}


/* Debug port (USART3) byte handler. Runs in the App task; bytes come from the
 * USART3 receive ring buffer (Usart/uart_rx.c). */
void APP_USART3_Byte(char data)
{
       {
       //USART_putc(USART3, data);

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

               if(data == 'b')
               {
            	   if(PLAYER1.score <= 0)
            	   {
            		   PLAYER1.score = 0;
            	   }
            	   else
                 PLAYER1.score--;
                 GAME.COMMAND = 1;
               }

               if(data == 'c')
               {
                 PLAYER2.score++;
                 GAME.COMMAND = 1;
               }
               if(data == 'd')
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
        		 	 GAME.STATUS = select_player1_mode;
	        		 SHIFT_PLAYERS(GAMER_ALL[id], id, PLAYER_1);
	        		 id++;
	        		 if(id > 10)
	        		 {
	        			 id = 0;
	        		 }
	        	 }

        	 if(data == 'c')
	        	 {
        		 	 GAME.STATUS = select_player2_mode;
	        		 SHIFT_PLAYERS(GAMER_ALL[id2], id2, PLAYER_2);
	        		 id2++;
	        		 if(id2 > 10)
	        		 {
	        			 id2 = 0;
	        		 }
	        	 }



             if(data == 'x')
             {
               PLAYER1 =  SAVING_GAMER_DATA2(PLAYER1.id, PLAYER1);
               PLAYER2 =  SAVING_GAMER_DATA2(PLAYER2.id, PLAYER2);

               DISPLAY_CURRENT_SCORE(PLAYER1, PLAYER2);   // DISPLAY PREVIOUS SCORE
             }



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

               if(data == 'a' || data == 'c')
               {
            	   GAME.POINTS ++;
            	   GAME_TYPE_1();
               }


	               if(data == 'z' || data == 'm')
	               {
	            	   GAME.POINTS --;
	            	   //GAME_TYPE_1();
	               }


         COMMAND_RECEIVER2(data);


       }

}


void COMMAND_RECEIVER2(char data)
{


  /* a start character restarts the frame; every byte is stored */
  if(data == '*' || data =='<')
  {
    in_count = 0;
  }
  inbuf_h[in_count++] = data;

  if(data == '#' || data == '>')
  {
    length2 = in_count;
    datalength2 = length2;
    in_count = 0;

    char Inbuf[datalength2 + 2];   /* [0]=length, [1..]=frame (the copy below writes datalength2+1) */

    for( j = 0; j<= datalength2; j++)
    {
    	Inbuf[j+1] = inbuf_h[j];
    }

    Inbuf[0] = length2;

    COMM.buffer_size = datalength2;



    char DATA_BUFFER[length2 -3];

    for(j = 0; j < sizeof(DATA_BUFFER); j++)
    {
    	DATA_BUFFER[j] = Inbuf[j+4];
    }



    COMM.buffer_size  = sizeof(DATA_BUFFER);
    COMM.STX 		  = Inbuf[1];
    COMM.CODE1 		  = Inbuf[2];
    COMM.CODE2 		  = Inbuf[3];
    COMM.ETX 		  = Inbuf[datalength2];


    DD_RECEIVER(DATA_BUFFER);

  }

}


void DD_RECEIVER(char* INBUF)
{

  uint16_t LENGTH = strlen(INBUF);

  char BUFFER[LENGTH + 1];


  strlcpy(BUFFER, INBUF, COMM.buffer_size);

  COMM.DATA = BUFFER;


  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'T' && COMM.CODE2 == 'M')
    {
	  STOP_TIMER(RTC_TIM_TYPE);
	  TIMER_UPDATE = 1;

	  uint8_t min = strtol(COMM.DATA, NULL, 16);
	  LOG_I("Clock minutes set to %d", min);

	  RTC_SetMinutes(min);          /* binary, as in the original */

	  TIMER_UPDATE = 0;
	  INIT_TIMER_CONFIG(RTC_TIMER_PERIPH, RTC_TIMER_IRQ, RTC_TIM_TYPE, RTC_UPDATE_TIME);
    }

  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'T' && COMM.CODE2 == 'H')
    {
	  STOP_TIMER(RTC_TIM_TYPE);
	  TIMER_UPDATE = 1;

	  uint8_t hour = strtol(COMM.DATA, NULL, 16);
	  LOG_I("Clock hours set to %02X", hour);

	  RTC_SetHours(hour);           /* BCD, as in the original */

	  TIMER_UPDATE = 0;
	  INIT_TIMER_CONFIG(RTC_TIMER_PERIPH, RTC_TIMER_IRQ, RTC_TIM_TYPE, RTC_UPDATE_TIME);
    }

  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'n' && COMM.CODE2 == '1')
    {

	  GAME.initial_players_name = 0;
  	  /*

      PLAYER1.name = content2;
      CHANGE_FONT(GREEN);
      calculateLEDString( PLAYER1.name, 0, 8, Font8x8);
      CHANGE_FONT(BLUE);
      */
	//  eol(USART3);
	//  USART_puts(USART3, "PLAYER1 NAME -> ");
	  //PLAYER1.name_M = PLAYER1.name;
	 strcpy(PLAYER1.name, COMM.DATA);
	 LOG_I("Player 1 name: %s", PLAYER1.name);
	 // USART_puts(USART3, PLAYER1.name);
	//  eol(USART3);
	  	  	CHANGE_FONT(GREEN);
	        calculateLEDString( PLAYER1.name, 0, 8, Font8x8);
	        CHANGE_FONT(BLUE);


    }

    if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'n' && COMM.CODE2 == '2')
    {
    	GAME.initial_players_name = 0;
  	  /*
  	    GAME.initial_players_name = 0;
  	    PLAYER2.name = content2;
  	    CHANGE_FONT(RED);
  	    calculateLEDString(PLAYER2.name, 72, 8, Font8x8);
  	    CHANGE_FONT(BLUE);
  	    */

  	  //eol(USART3);
  	 // USART_puts(USART3, "PLAYER2 NAME -> ");
      PLAYER2.name_M = PLAYER2.name;
  	  PLAYER2.name_M = COMM.DATA;
  	//  USART_puts(USART3, PLAYER2.name);
  	//  eol(USART3);

  			CHANGE_FONT(RED);
  	  	    calculateLEDString(PLAYER2.name, 72, 8, Font8x8);
  	  	    CHANGE_FONT(BLUE);

    }


    if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'm' && COMM.CODE2 == '1')
    {
	 // PLAYER1.message = COMM.DATA;
    	strcpy(PLAYER1.message, COMM.DATA);
    	LOG_I("Player 1 message: %s", PLAYER1.message);
	    PLAYER1 = SAVING_GAMER_DATA2(1, PLAYER1);

    }

      if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'm' && COMM.CODE2 == '2')
    {


    }

     if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'g' && COMM.CODE2 == 'm')
    {
    	  /* static copy: COMM.DATA points into this function's stack buffer */
    	  static char general_message[64];
    	  strlcpy(general_message, COMM.DATA, sizeof general_message);
    	  GAME.MESSAGE = general_message;
    	  LOG_I("Message: %s", GAME.MESSAGE);

  	 // GAME.message = content2;
    }

}








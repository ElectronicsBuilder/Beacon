


#include "Comm.h"
#include "TIM_CTRL.h"
#include "matrix.h"
#include "GAME.h"

#include "Ez_USART.h"
#include "app_main.h"
#include "FONT.h"
#include "rtc_clock.h"

uint16_t id, id2;


#ifdef OLD_DATA_RECEIVER

void D_RECEIVER(char*INBUF)
{

  COMM.buffer_size = inbuf[0];

  DISPLAY_INT2(USART3, COMM.buffer_size, 10, 1);

  if(COMM.buffer_size == 0)
  {
	  COMM.buffer_size = 5; // Handle a null buffer.
  }


  unsigned char content[COMM.buffer_size];

  for(i = 0; i < COMM.buffer_size; i++)
  {
    content[i] = inbuf[i+1];
  }

  COMM.STX = content[0];
  COMM.CODE1 = content[1];
  COMM.CODE2 = content[2];
  COMM.ETX = content[sizeof(content)-1];


  uint16_t str_lenght1 = strlen(content);

  USART_puts(USART3, "CONTENT1 LENGTH ->  ");
  DISPLAY_INT2(USART3, str_lenght1, 10, 1);





unsigned char content2[COMM.buffer_size];

  for(k = 0; k < sizeof(content); k++)
  {
    content2[k] = content[k+3];
  }






  USART_putc(USART3,sizeof(content2));

  USART_putc(USART3,COMM.STX);
  USART_putc(USART3,COMM.CODE1);
  USART_putc(USART3,COMM.CODE2);

  for(m = 0; m < sizeof(content2)-1; m++)
  {
    USART_putc(USART3,content2[m]);
  }
  USART_putc(USART3,COMM.ETX);
  eol(USART3);



  uint16_t str_lenght = strlen(content2);

  USART_puts(USART3, "CONTENT LENGTH ->  ");
  DISPLAY_INT2(USART3, str_lenght, 10, 0);
  eol(USART3);


  USART_puts(USART3, "CONTENT ->  ");
  USART_puts(USART3, content2);
  USART_puts(USART3, "  <- END");


  eol(USART3);







  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'n' && COMM.CODE2 == '1')
  {
	GAME.initial_players_name = 0;
    PLAYER1.name = content2;
    CHANGE_FONT(GREEN);
    calculateLEDString( PLAYER1.name, 0, 8, Font8x8);
    CHANGE_FONT(BLUE);

  }

  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'n' && COMM.CODE2 == '2')
  {
	    GAME.initial_players_name = 0;
	    PLAYER2.name = content2;
	    CHANGE_FONT(RED);
	    calculateLEDString(PLAYER2.name, 72, 8, Font8x8);
	    CHANGE_FONT(BLUE);

  }


  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'm' && COMM.CODE2 == '1')
  {
	  PLAYER1.message = content2;
  }

  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'm' && COMM.CODE2 == '2')
  {
	  PLAYER2.message = content2;
  }


  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'g' && COMM.CODE2 == 'm')
  {
	  GAME.message = content2;
  }




  /*



   if(COMM.STX == '<' && COMM.ETX == '>')
  {
    COMM.COMMAND = content2;
    COMMAND_PROCESS(COMM.COMMAND);

  }

  */

}

void COMMAND_RECEIVER(char data)
{


  if(data == '*' || data =='<')

    in_count = 0;
    inbuf_h[in_count++] = data;

  if(data == '#' || data == '>')
  {
    length2 = in_count;
    datalength2 = length2;
    in_count = 0;

    for( j = 0; j<= datalength2; j++)
    {
      inbuf[j+1] = inbuf_h[j];
    }

    inbuf[0] = length2;

    for( j = 0; j<= datalength2; j++)
    {
      USART_putc(USART3, inbuf[j]);
    }
//
    for( j = datalength2+1; j <= sizeof(inbuf-1); j++)
    {
      inbuf[j] = NULL;
    }

    eol(USART3);

    D_RECEIVER(inbuf);

  }

}

void DATA_PROCESS(char*DATA)
{
	int status;

	for( i = 0; i < sizeof(DATA); i++)
	{
		if(DATA[i] == CONSOLE_INIT_REQ[i])
		{
			status = init_success;
		}

		if(DATA[i] == CONSOLE_SEND_TIME[i])
		{
			status = time_success;
		}

		else
		{
			status = failed;
		}

	}

	USART_puts(USART3,status);
}

void COMMAND_PROCESS(char*DATA)
{

}
#endif



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


  if(data == '*' || data =='<')

    in_count = 0;
    inbuf_h[in_count++] = data;

  if(data == '#' || data == '>')
  {
    length2 = in_count;
    datalength2 = length2;
    in_count = 0;

    char Inbuf[datalength2];

    for( j = 0; j<= datalength2; j++)
    {
    	Inbuf[j+1] = inbuf_h[j];
    }

    Inbuf[0] = length2;

    for( j = 0; j<= datalength2; j++)
    {
      USART_putc(USART3, Inbuf[j]);
    }

    for( j = datalength2+2; j <= sizeof(Inbuf-1); j++)
    {
      Inbuf[j] = " ";
    }

    COMM.buffer_size = datalength2;
    eol(USART3);



    char DATA_BUFFER[length2 -3];

    for(j = 0; j < sizeof(DATA_BUFFER); j++)
    {
    	DATA_BUFFER[j] = Inbuf[j+4];
    }

    eol(USART3);

    USART_puts(USART3, DATA_BUFFER);

    eol(USART3);


    COMM.buffer_size  = sizeof(DATA_BUFFER);
    COMM.STX 		  = Inbuf[1];
    COMM.CODE1 		  = Inbuf[2];
    COMM.CODE2 		  = Inbuf[3];
    COMM.ETX 		  = Inbuf[datalength2];

    USART_putc(USART3, COMM.buffer_size );
    USART_putc(USART3, COMM.STX );
    USART_putc(USART3,  COMM.CODE1);
    USART_putc(USART3, COMM.CODE2);
    USART_putc(USART3, COMM.ETX );


    eol(USART3);

    DD_RECEIVER(DATA_BUFFER);

  }

}

uint8_t HexToInt(char h)
    {
      if ((h >= '0') && (h <= '9'))
      {
        return (h - '0');
      }
      if ((h >= 'a') && (h <= 'f'))
      {
        return ((h - 'a') + 10);
      }
      if ((h >= 'A') && (h <= 'F'))
      {
        return ((h - 'A') + 10);
      }
      return -1;
    }

void DD_RECEIVER(char* INBUF)
{

  uint16_t LENGTH = strlen(INBUF);

  unsigned char BUFFER[LENGTH];


  DISPLAY_INT2(USART3, LENGTH, 10, 1);

  strlcpy(BUFFER, INBUF, COMM.buffer_size);

  COMM.DATA = BUFFER;
  USART_puts(USART3, COMM.DATA);


  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'T' && COMM.CODE2 == 'M')
    {
	  STOP_TIMER(RTC_TIM_TYPE);
	  TIMER_UPDATE = 1;

	  uint8_t min = strtol(COMM.DATA, NULL, 16);
	  DISPLAY_INT2(USART3, min, 10, 1);

	  RTC_SetMinutes(min);          /* binary, as in the original */

	  TIMER_UPDATE = 0;
	  INIT_TIMER_CONFIG(RTC_TIMER_PERIPH, RTC_TIMER_IRQ, RTC_TIM_TYPE, RTC_UPDATE_TIME);
    }

  if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'T' && COMM.CODE2 == 'H')
    {
	  STOP_TIMER(RTC_TIM_TYPE);
	  TIMER_UPDATE = 1;

	  uint8_t hour = strtol(COMM.DATA, NULL, 16);
	  DISPLAY_INT2(USART3, hour, 10, 1);

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
	    PLAYER1 = SAVING_GAMER_DATA2(1, PLAYER1);

    }

      if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'm' && COMM.CODE2 == '2')
    {


    }

     if(COMM.STX == '*' && COMM.ETX == '#' && COMM.CODE1 == 'g' && COMM.CODE2 == 'm')
    {
    	  eol(USART3);
    	  USART_puts(USART3, "GENERAL MESSAGE -> ");
    	  GAME.MESSAGE = COMM.DATA;
    	  USART_puts(USART3, GAME.MESSAGE);
    	  eol(USART3);

  	 // GAME.message = content2;
    }

}








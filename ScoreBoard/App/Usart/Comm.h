#ifndef __COMM_H
#define __COMM_H




void APP_USART3_Byte(char data);   /* debug port command byte (App task) */
void APP_USART2_Byte(char data);   /* radio command byte (App task) */

void DD_RECEIVER(char* INBUF);

void COMMAND_RECEIVER2(char data);



#endif

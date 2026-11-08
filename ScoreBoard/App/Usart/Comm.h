#ifndef __COMM_H
#define __COMM_H




void APP_USART3_IRQ(void);   /* debug port RX byte handler */
void APP_USART2_IRQ(void);   /* radio RX byte handler */

void DD_RECEIVER(char* INBUF);

void COMMAND_RECEIVER2(char data);



#endif

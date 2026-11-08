

#ifndef _TIM_CTRL_H
#define  _TIM_CTRL_H
#include "app_main.h"


uint16_t UPDATE_COUNTER, UPDATE_GO, SCREEN_SLEEP, SCREEN_SLEEP_MIN, SLEEP_STAT;

/* Original timer interrupt bodies, run as App-task jobs by AppTimers_Run() */
void APP_TIM2_Tick(void);   /* RTC clock refresh (1 s)      */
void APP_TIM3_Tick(void);   /* on-the-fly messages (30 ms)  */
void APP_TIM4_Tick(void);   /* display update (100 ms)      */
void APP_TIM5_Tick(void);   /* LED matrix data update        */
void APP_TIM7_Tick(void);   /* USB keyboard poll (1 ms)     */
void STOP_UPDATE(void);
void RESUME_UPDATE(void);

void KEYBOARD_CONTROL(uint8_t data);

/* Colours of the player names on the top line of the matrix.
 * Workaround for a wiring fault on the top half of the panels (header pins 2-3):
 * the panel's blue input is stuck on, PC6 shows up as green and PC7 (GREEN)
 * is not connected. Readable choices there: RED (shows pink), BLUE (shows
 * cyan) and WHITE. Once the wiring is fixed any colour works again. */
#define NAME_SERVER_COLOR    BLUE
#define NAME_RECEIVER_COLOR  RED
#define NAME_SELECT_P1_COLOR BLUE
#define NAME_SELECT_P2_COLOR RED
void RESUME_DATA_UPDATE(void);
void STOP_DATA_UPDATE(void);
void DISPLAY_SLEEP(void);

void RESUME_UPDATE_DISPLAY(void);
void STOP_UPDATE_DISPLAY(void);




#endif


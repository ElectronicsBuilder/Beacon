//--------------------------------------------------------------
// File     : app_main.c
// Scoreboard application (originally main.c of the 2016 CoIDE project).
//
// App_Init(): start-up sequence (OLED, EEPROM, RTC, players, LED matrix,
//             periodic jobs). App_Run(): App task main loop.
// Both run in the FreeRTOS default task (Core/Src/freertos.c).
//--------------------------------------------------------------
#include "app_hal.h"
#include "app_main.h"
#include "Ez_USART.h"
#include "status_led.h"
#include "dig_out.h"
#include "delay.h"
#include "game_clock.h"
#include "EEPROM.h"
#include "i2c_selftest.h"
#include "GAME.h"
#include "FONT.h"
#include "matrix.h"
#include "bitmaps.h"
#include "TIM_CTRL.h"
#include "app_timers.h"
#include "SSD1351.h"
#include "usb_keyboard.h"
#include "uart_rx.h"
#include "settings.h"
#include "log.h"
#include "cli.h"
#include "commands.h"
#include "mode.h"
#include "rtc_trim.h"
#include "countdown.h"
#include "msg_store.h"
#include "version.h"
#include "cmsis_os.h"

/* Global variables declared in app_main.h (defined here once) */
uint8_t c;
int i, j, m, n;
uint16_t in_count, datalength2, length2;
char inbuf_h[50];
unsigned char TIMER_UPDATE;
uint8_t SEC_H, MIN_H, HOUR_H;
char D_SEC[50];
char D_MIN[50];
char D_HOUR[50];
char number3[50];
char number4[50];
char number5[50];
char number6[50];
/* Free stack of the App task in bytes (lowest seen), readable over the
 * debugger while testing. */
volatile uint32_t app_task_stack_free;

/*
 * Start-up sequence of the original firmware (was main()).
 * Runs once from the FreeRTOS default task after the CubeMX init.
 */
void App_Init(void)
{
  DELAY_INIT();
  Led_Init();
  DigOut_Init();
  DigOut_Lo(RADIO_EN);
  DigOut_Lo(EE_WP);             // EEPROM writable
  DigOut_Hi(RADIO_CONFIG);
  Led_On(LED_GREEN);

  UsbKbd_Init();
  USART_INIT(USART3, 0, 0, 0, 0, 115200, USART3_IRQn);   // debug console, RX interrupt on
  USART_INIT(USART2, 0, 0, 0, 0, 19200, USART2_IRQn);    // radio, RX interrupt on
  LOG_I("ScoreBoard firmware %s starting", FW_VERSION);

  I2C_SelfTest();               // EEPROM wiring test (off unless APP_I2C_SELFTEST), before TFT_INIT

  /* OLED splash */
  TFT_INIT();
  set_rotation(0);
  SSD1351_fillScreen(RGB_COL_BLACK);
  TFT_DrawBMP(0, 0, &trooper);

  RETURN_RANDOM_SERVER();

  EE_TEST();
  Settings_Load();              // saved brightness and display mode
  RtcTrim_Init();               // RTC accuracy: LSI measured against the crystal
  RTC_Configuration();

  /* OLED start-up check list */
  OledFont_DrawString(0, 0,  "BEACON", &Arial_7x10, RGB_COL_BLUE,    RGB_COL_BLACK);
  OledFont_DrawString(0, 15, "I2C EE INIT CHECK",  &Arial_7x10, RGB_COL_GREEN,   RGB_COL_BLACK);
  RTC_Configuration();
  OledFont_DrawString(0, 30, "RTC INIT CHECK",     &Arial_7x10, RGB_COL_YELLOW,  RGB_COL_BLACK); DELAY_MILLI(100);
  OledFont_DrawString(0, 45, "MATRIX INIT CHECK",  &Arial_7x10, RGB_COL_CYAN,    RGB_COL_BLACK); DELAY_MILLI(100);
  OledFont_DrawString(0, 60, "USB HID INIT CHECK", &Arial_7x10, RGB_COL_MAGENTA, RGB_COL_BLACK); DELAY_MILLI(100);
  OledFont_DrawString(0, 75, "SYS INIT CHECK ",    &Arial_7x10, RGB_COL_BLUE,    RGB_COL_BLACK); DELAY_MILLI(100);
  OledFont_DrawString(0, 90, "TFT INIT CHECK ",    &Arial_7x10, RGB_COL_BLUE,    RGB_COL_BLACK); DELAY_MILLI(100);

  /* Player records from the EEPROM */
  TEST_GAMERS2(GAMER0, 0);
  TEST_GAMERS2(GAMER1, 1);
  TEST_GAMERS2(GAMER2, 2);
  TEST_GAMERS2(GAMER3, 3);
  TEST_GAMERS2(GAMER4, 4);
  TEST_GAMERS2(GAMER5, 5);
  TEST_GAMERS2(GAMER6, 6);
  TEST_GAMERS2(GAMER7, 7);
  TEST_GAMERS2(GAMER8, 8);
  TEST_GAMERS2(GAMER9, 9);
  TEST_GAMERS2(GAMER10, 10);
  DELAY_MILLI(100);
  EXTR_PLAYER_DATA();
  DELAY_MILLI(1000);

  /* Initial game state */
  GAME.DISPLAY_WINNER = "CURRENT WINNER   : ";
  GAME.MESSAGE = "NO MSG";
  GAME.initial_players_name = 1;
  GAME.TOGGLE_WINNER = 0;
  GAME.STATUS = idle;
  GAME.START = 0;

  /* LED matrix: logos, then "BEACON" */
  initLEDdata();
  calculateClearImage();
  updateLEDData();
  CHANGE_FONT(BLUE);
  UG_DrawBMP(0, 0, &logo);
  UG_DrawBMP(96, 0, &logo2);
  calculateLEDdata();
  DELAY_MILLI(1000);

  calculateClearImage();
  updateLEDData();
  CHANGE_FONT(BLUE);
  calculateLEDString("BEACON", 0, 0, Font8x8);
  CHANGE_FONT(RED);
  calculateLEDString("LLC", 32, 16, Font8x8);
  calculateLEDdata();
  DELAY_MILLI(1000);

  calculateClearImage();
  updateLEDData();
  calculateLEDdata();
  CHANGE_FONT(BLUE);

  /* Display mode (ping-pong, clock, ...), the one saved in the EEPROM */
  Timer_Load();                 // countdown / count-up settings
  MsgStore_Load();              // message list
  Mode_Init();

  /* Console on USART3 */
  CmdSystem_Register();
  CmdGame_Register();
  CmdTimer_Register();
  CmdMsg_Register();
  Cli_Init();
}

/*
 * App task main loop (runs in the FreeRTOS default task after App_Init()).
 * Every millisecond: handle received serial bytes and keyboard keys, run the
 * active display mode, refresh the LED matrix, save changed settings.
 */
#define MATRIX_REFRESH_MS 4u

void App_Run(void)
{
  uint32_t next = osKernelGetTickCount();
  uint32_t loops = 0;
  for (;; loops++)
  {
    next += 1u;
    if ((int32_t)(osKernelGetTickCount() - next) > 10)
    {
      next = osKernelGetTickCount();   /* a job ran long: resynchronise */
    }
    osDelayUntil(next);
    UartRx_Poll();
    Mode_Key(UsbKbd_GetKey());
    Mode_Tick();
    if (loops % MATRIX_REFRESH_MS == 0u)
    {
      Matrix_Refresh();            /* rebuilds the DMA data only if the image changed */
    }
    Settings_Poll();
    RtcTrim_Poll();
    Timer_Poll();
    if (loops % 1000u == 0u)
    {
      app_task_stack_free = osThreadGetStackSpace(osThreadGetId());
    }
  }
}

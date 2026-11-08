//--------------------------------------------------------------
// File     : app_main.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdio.h>
#include <string.h>
#include "app_hal.h"
#include "usb_keyboard.h"
#include "status_led.h"

//======================================================================
// global defines
//----------------------------------------------------------------------
#define OK       (0)
/* ERROR comes from the HAL (ErrorStatus); the old -1 value was only used by the I2C code. */

//======================================================================
// Definition of my favorite types
//----------------------------------------------------------------------
typedef signed char        s08;
typedef signed short       s16;
typedef int32_t            s32;
typedef signed long long   s64;
typedef unsigned char      u08;
typedef unsigned short     u16;
typedef uint32_t           u32;
typedef unsigned long long u64;




uint8_t c;

int i, j, k, l, m, n, o, p, x;
 uint8_t data;
 uint16_t in_count, datalength2, length2;

#define RED 0
#define GREEN 1
#define BLUE 2
#define WHITE 3
#define SERVE_COLOR 4
#define MTX_YELLOW 5      /* red + green LEDs */


 unsigned char TIMER_UPDATE;
 unsigned char TEMP_SW_COM;

 uint8_t SEC_H, MIN_H, HOUR_H;
 uint8_t SEC_C, MIN_C, HOUR_C;
 uint16_t cnt;

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------



#define __UG_CONST   const
typedef uint8_t      UG_U8;
typedef int8_t       UG_S8;
typedef uint16_t     UG_U16;
typedef int16_t      UG_S16;
typedef uint32_t     UG_U32;
typedef int32_t      UG_S32;













typedef UG_U32                                        UG_COLOR;
#define BMP_BPP_1							(1<<0)


#define BMP_BPP_2                                     (1<<1)
#define BMP_BPP_4                                     (1<<2)
#define BMP_BPP_8                                     (1<<3)
#define BMP_BPP_16                                    (1<<4)
#define BMP_BPP_32                                    (1<<5)
#define BMP_RGB888                                    (1<<0)
#define BMP_RGB565                                    (1<<1)
#define BMP_RGB555                                    (1<<2)

typedef struct
{
   void* p;
   UG_U16 width;
   UG_U16 height;
   UG_U8 bpp;
   UG_U8 colors;
} EZ_BMP;







 char buffer[50];
 char D_SEC[50];
 char D_MIN[50];
 char D_HOUR[50];
 uint16_t counter, counter1, counter2, counter3,h_c, FLAGS;
 char number1[50];
 char number2[50];

 char number3[50];
 char number4[50];

 char number5[50];
 char number6[50];

 char numberscore1[50];
 char numberscore2[50];

 uint16_t in_count, datalength, d_count ;
 unsigned char length;
 char inbuf[50];
 char inbuf_h[50];
 char HOLDER[2];



 //USB

 uint8_t USB_KEYBOARD;
 USB_HID_HOST_STATUS_t host_status;
  int status;

  uint8_t key_function(void);
  uint16_t  POLL_USB(void);

  uint16_t timer_count;

/*
 * RGB 323 8BIT
 *
 *https://msdn.microsoft.com/en-us/library/windows/desktop/dd390989%28v=vs.85%29.aspx
 *        UG_COLOR red_mask = 0xE0;
         UG_COLOR green_mask = 0x18;
         UG_COLOR blue_mask = 0x07;

         UG_U8 r_val = (tmp & red_mask) >> 5;
         UG_U8 g_val = (tmp & green_mask) >> 3;
         UG_U8 b_val = (tmp & blue_mask);

         foreground_color.R= r_val;
         foreground_color.G= g_val;
         foreground_color.B= b_val;

 */






//--------------------------------------------------------------
#endif // __APP_MAIN_H

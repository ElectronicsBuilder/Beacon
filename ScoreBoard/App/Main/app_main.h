//--------------------------------------------------------------
// File     : app_main.h
// Shared types, colour codes and globals of the scoreboard application.
//--------------------------------------------------------------
#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdio.h>
#include <string.h>
#include "app_hal.h"
#include "usb_keyboard.h"
#include "status_led.h"

#define OK       (0)
/* ERROR comes from the HAL (ErrorStatus). */

/* Short integer types used by the original code */
typedef signed char        s08;
typedef signed short       s16;
typedef int32_t            s32;
typedef signed long long   s64;
typedef unsigned char      u08;
typedef unsigned short     u16;
typedef uint32_t           u32;
typedef unsigned long long u64;

/* LED matrix font colours for CHANGE_FONT() (Matrix/matrix.c) */
#define RED          0
#define GREEN        1
#define BLUE         2
#define WHITE        3
#define SERVE_COLOR  4
#define MTX_YELLOW   5      /* red + green LEDs */

/* Bitmap types (start-up images, Matrix/bitmaps.c) */
#define __UG_CONST   const
typedef uint8_t      UG_U8;
typedef int8_t       UG_S8;
typedef uint16_t     UG_U16;
typedef int16_t      UG_S16;
typedef uint32_t     UG_U32;
typedef int32_t      UG_S32;
typedef UG_U32       UG_COLOR;

#define BMP_BPP_1    (1<<0)
#define BMP_BPP_2    (1<<1)
#define BMP_BPP_4    (1<<2)
#define BMP_BPP_8    (1<<3)
#define BMP_BPP_16   (1<<4)
#define BMP_BPP_32   (1<<5)
#define BMP_RGB888   (1<<0)
#define BMP_RGB565   (1<<1)
#define BMP_RGB555   (1<<2)

typedef struct
{
   void* p;
   UG_U16 width;
   UG_U16 height;
   UG_U8 bpp;
   UG_U8 colors;
} EZ_BMP;

/* Globals shared by the original modules (defined in app_main.c) */
extern uint8_t c;
extern int i, j, m, n;                                  /* shared loop counters */
extern uint16_t in_count, datalength2, length2;         /* serial frame parser */
extern char inbuf_h[50];
extern unsigned char TIMER_UPDATE;
extern uint8_t SEC_H, MIN_H, HOUR_H;                    /* game clock (BCD) */
extern char D_SEC[50], D_MIN[50], D_HOUR[50];           /* clock text */
extern char number3[50], number4[50], number5[50], number6[50];   /* score text */
extern uint8_t USB_KEYBOARD;
extern USB_HID_HOST_STATUS_t host_status;
extern uint16_t timer_count;

uint8_t key_function(void);
uint16_t POLL_USB(void);

#endif // APP_MAIN_H

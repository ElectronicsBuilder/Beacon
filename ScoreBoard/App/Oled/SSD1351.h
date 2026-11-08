

//--------------------------------------------------------------
#ifndef __SSD1351_H
#define __SSD1351_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "app_hal.h"
#include "oled_font.h"

#include "app_main.h"
//--------------------------------------------------------------
// The reset line can be disabled here
// (if unused, the display RES pin must be tied to Vcc)
//--------------------------------------------------------------
//#define  USE_SSD1331_RESET  0  // do not use the reset pin
#define  USE_SSD1331_RESET  1  // use the reset pin

//--------------------------------------------------------------
// Display defines
//--------------------------------------------------------------
/*
#define  LCD_MAXX           ((uint16_t)96)      // pixels in X
#define  LCD_MAXY           ((uint16_t)64)      // pixels in Y
#define  LCD_PIXEL  LCD_MAXX*LCD_MAXY
*/


#define SSD1351WIDTH 128
#define SSD1351HEIGHT 128  // SET THIS TO 96 FOR 1.27"!

#define swap(a, b) { uint16_t t = a; a = b; b = t; }

#define SSD1351_COLORORDER_RGB

#define SSD1351_DELAYS_HWFILL	    (3)
#define SSD1351_DELAYS_HWLINE       (1)

#define PORTRAIT 0
#define UP 1
#define DOWN 0

#define  LCD_MAXX           ((uint16_t)128)      // pixels in X
#define  LCD_MAXY           ((uint16_t)128)      // pixels in Y
#define  LCD_PIXEL  LCD_MAXX*LCD_MAXY


//--------------------------------------------------------------
// Standard colours
// 16-bit colour (R5G6B5)
// Red   (5bit) -> Bit15-Bit11
// Green (6bit) -> Bit10-Bit5
// Blue  (5bit) -> Bit4-Bit0
//--------------------------------------------------------------
#define  RGB_COL_BLACK          0x0000
#define  RGB_COL_BLUE           0x001F
#define  RGB_COL_GREEN          0x07E0
#define  RGB_COL_RED            0xF800
#define  RGB_COL_WHITE          0xFFFF

#define  RGB_COL_CYAN           0x07FF
#define  RGB_COL_MAGENTA        0xF81F
#define  RGB_COL_YELLOW         0xFFE0

#define  RGB_COL_GREY           0xF7DE



//--------------------------------------------------------------
// Other defines
//--------------------------------------------------------------
#define  LCD_INIT_PAUSE   2000000  // short pause during init
#define  LCD_SPI_PAUSE    10       // short pause for SPI




//--------------------------------------------------------------
// SPI Chip-Select-Pin
//--------------------------------------------------------------
#define LCD_CS_PIN             OLED_CS_Pin        // PD9
#define LCD_CS_GPIO_PORT       OLED_CS_GPIO_Port

//--------------------------------------------------------------
// DC-Pin
//--------------------------------------------------------------
#define LCD_DC_PIN            OLED_DC_Pin         // PD10
#define LCD_DC_GPIO_PORT      OLED_DC_GPIO_Port

//--------------------------------------------------------------
// Reset-Pin
//--------------------------------------------------------------
#if USE_SSD1331_RESET==1
  #define LCD_RESET_PIN            OLED_RES_Pin     // PB8
  #define LCD_RESET_GPIO_PORT      OLED_RES_GPIO_Port
#endif



//--------------------------------------------------------------
// SSD1331 registers
//--------------------------------------------------------------
 
#define  SSD1331_CMD_CONTRASTA      0x81
#define  SSD1331_CMD_CONTRASTB      0x82
#define  SSD1331_CMD_CONTRASTC      0x83
#define  SSD1331_CMD_MASTERCURRENT  0x87
#define  SSD1331_CMD_PRECHARGEA     0x8A
#define  SSD1331_CMD_PRECHARGEB     0x8B
#define  SSD1331_CMD_SETREMAP       0xA0
#define  SSD1331_CMD_STARTLINE      0xA1
#define  SSD1331_CMD_DISPLAYOFFSET  0xA2
#define  SSD1331_CMD_NORMALDISPLAY  0xA4
#define  SSD1331_CMD_SETMULTIPLEX   0xA8
#define  SSD1331_CMD_SETMASTER      0xAD
#define  SSD1331_CMD_DISPLAYOFF     0xAE
#define  SSD1331_CMD_DISPLAYON      0xAF
#define  SSD1331_CMD_POWERMODE      0xB0
#define  SSD1331_CMD_PRECHARGE      0xB1
#define  SSD1331_CMD_CLOCKDIV       0xB3
#define  SSD1331_CMD_PRECHARGELEVEL 0xBB
#define  SSD1331_CMD_VCOMH          0xBE


//--------------------------------------------------------------
// SSD1331 commands
//--------------------------------------------------------------

#define  SSD1331_CMD_COLUMN         0x15
#define  SSD1331_CMD_ROW            0x75
#define  SSD1331_CMD_LINE           0x21


#define  SSD1331_CMD_RECT           0x22  // dont work (bug ?)
#define  SSD1331_CMD_FILL           0x26  // dont work (bug ?)


//--------------------------------------------------------------
// Font (max 16 pixels wide)
//-------------------------------------------------------------- 


// SSD1351 Commands
#define SSD1351_CMD_SETCOLUMN 		0x15
#define SSD1351_CMD_SETROW    		0x75
#define SSD1351_CMD_WRITERAM   		0x5C
#define SSD1351_CMD_READRAM   		0x5D
#define SSD1351_CMD_SETREMAP 		0xA0
#define SSD1351_CMD_STARTLINE 		0xA1
#define SSD1351_CMD_DISPLAYOFFSET 	0xA2
#define SSD1351_CMD_DISPLAYALLOFF 	0xA4
#define SSD1351_CMD_DISPLAYALLON  	0xA5
#define SSD1351_CMD_NORMALDISPLAY 	0xA6
#define SSD1351_CMD_INVERTDISPLAY 	0xA7
#define SSD1351_CMD_FUNCTIONSELECT 	0xAB
#define SSD1351_CMD_DISPLAYOFF 		0xAE
#define SSD1351_CMD_DISPLAYON     	0xAF
#define SSD1351_CMD_PRECHARGE 		0xB1
#define SSD1351_CMD_DISPLAYENHANCE	0xB2
#define SSD1351_CMD_CLOCKDIV 		0xB3
#define SSD1351_CMD_SETVSL 			0xB4
#define SSD1351_CMD_SETGPIO 		0xB5
#define SSD1351_CMD_PRECHARGE2 		0xB6
#define SSD1351_CMD_SETGRAY 		0xB8
#define SSD1351_CMD_USELUT 			0xB9
#define SSD1351_CMD_PRECHARGELEVEL 	0xBB
#define SSD1351_CMD_VCOMH 			0xBE
#define SSD1351_CMD_CONTRASTABC		0xC1
#define SSD1351_CMD_CONTRASTMASTER	0xC7
#define SSD1351_CMD_MUXRATIO            0xCA
#define SSD1351_CMD_COMMANDLOCK         0xFD
#define SSD1351_CMD_HORIZSCROLL         0x96
#define SSD1351_CMD_STOPSCROLL          0x9E
#define SSD1351_CMD_STARTSCROLL         0x9F

/*
typedef struct OledFont_t {
  const uint16_t *table; // glyph data table
  uint16_t width;        // character width (pixels)
  uint16_t height;       // character height (pixels)
}OledFont;
*/

//--------------------------------------------------------------
// Fonts available to the application
// (comment out unused ones)
// each font used needs its C file from App/Fonts
// in the build (listed in CMakeLists.txt)
//--------------------------------------------------------------
extern OledFont Arial_7x10;
extern OledFont FONT_4x6;
extern OledFont Font_6x8;

uint8_t rotation, TFT_WIDTH, TFT_HEIGHT;

//--------------------------------------------------------------
// Public functions
//--------------------------------------------------------------
void OLED_Init(void);
void OLED_SetCursor2Draw(uint8_t x, uint8_t y);
void OLED_SetPixel(uint16_t color);
void OLED_DrawPixel(uint16_t color);   // pixel at the current cursor position
void OLED_DrawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color);
void OLED_DrawChar(uint8_t x, uint8_t y, uint8_t ascii, OledFont *font, uint16_t vg, uint16_t bg);
void OLED_DrawString(uint8_t x, uint8_t y,char *ptr, OledFont *font, uint16_t vg, uint16_t bg);



void set_rotation(uint8_t x);
uint16_t getRotation(void);
uint16_t SSD1351_Color565(uint8_t r, uint8_t g, uint8_t b);
void SSD1351_fillScreen(uint16_t fillcolor);
void SSD1351_rawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fillcolor);
void SSD1351_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fillcolor) ;
void SSD1351_drawPixel(int16_t x, int16_t y, uint16_t color);
void TFT_INIT(void);
void TFT_DrawBMP( UG_S16 xp, UG_S16 yp, const EZ_BMP *bmp );
//--------------------------------------------------------------







/* Low-level helpers used inside the driver */
void P_SSD1351_InitIO(void);
void SSD1351_init(void);
void SSD1351_goTo(int x, int y);
void writeCommand(uint8_t wert);
void writeData(uint8_t wert);
void SSD1351_SetPixel(uint16_t color);
void SSD1351_DrawChar(uint8_t x, uint8_t y, uint8_t ascii, OledFont *font, uint16_t vg, uint16_t bg);

#endif // __SSD1351_H

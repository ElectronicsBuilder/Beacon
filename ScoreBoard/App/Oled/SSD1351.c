//--------------------------------------------------------------
// File     : SSD1351.c
// SSD1351 128x128 RGB OLED driver (1.5" display, 4-wire SPI)
//
// Pins (see ScoreBoard.ioc):
//   PA5  SPI1_SCK  -> display SCK      PD9  OLED_CS  -> display CS
//   PB5  SPI1_MOSI -> display SI       PD10 OLED_DC  -> display D/C
//   PB8  OLED_RES  -> display RES
// SPI: mode 3, MSB first, 21 MHz
//--------------------------------------------------------------

#include "SSD1351.h"
#include "oled_spi.h"
#include "Ez_USART.h"
#include "delay.h"
#include "oled_font.h"

/* Global variables declared in SSD1351.h (defined here once) */
uint8_t rotation, TFT_WIDTH, TFT_HEIGHT;




//--------------------------------------------------------------
// Internal functions
//--------------------------------------------------------------
void P_SSD1331_CS(BitAction wert);
void P_SSD1331_DC(BitAction wert);
void P_SSD1331_RES(BitAction wert);
void P_SSD1331_Delay(volatile uint32_t nCount);


//--------------------------------------------------------------
// Initialise the OLED display
//--------------------------------------------------------------
void OLED_Init(void)
{ 
  // initialise all IO pins
	P_SSD1351_InitIO();
  // initialise the SPI bus
  OledSpi_Init(SPI_MODE_3);
  // LCD-Controller initialisieren

  SSD1351_init();
 // P_SSD1331_InitChip();
}


//--------------------------------------------------------------
// Sets the cursor to x,y
//--------------------------------------------------------------
void OLED_SetCursor2Draw(uint8_t x, uint8_t y)
{
	/*
  if(xpos>=LCD_MAXX) xpos=LCD_MAXX-1;
  if(ypos>=LCD_MAXY) ypos=LCD_MAXY-1;

  // set cursor
  P_SSD1331_CMD(SSD1331_CMD_COLUMN);
  P_SSD1331_CMD(xpos);
  P_SSD1331_CMD(LCD_MAXX-1);

  P_SSD1331_CMD(SSD1331_CMD_ROW);
  P_SSD1331_CMD(ypos);
  P_SSD1331_CMD(LCD_MAXY-1);


  */



	  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT)) return;

	  // set x and y coordinate
	  writeCommand(SSD1351_CMD_SETCOLUMN);
	  writeData(x);
	  writeData(SSD1351WIDTH-1);

	  writeCommand(SSD1351_CMD_SETROW);
	  writeData(y);
	  writeData(SSD1351HEIGHT-1);

	  writeCommand(SSD1351_CMD_WRITERAM);
}

//--------------------------------------------------------------
// Draws one pixel at the current cursor position
//--------------------------------------------------------------
void OLED_SetPixel(uint16_t color)
{
	/*
  uint8_t hi,lo;
  // convert colour
  hi=(uint8_t)((color >> 8) & 0xFF);
  lo=(uint8_t)(color & 0xFF);
  // draw pixel
  P_SSD1331_DATA(hi);
  P_SSD1331_DATA(lo);
  */
	  P_SSD1331_DC(Bit_SET);
	  P_SSD1331_CS(RESET);

	  OledSpi_SendByte(color >> 8);
	  OledSpi_SendByte(color);

	  P_SSD1331_CS(Bit_SET);

}

void SSD1351_SetPixel(uint16_t color)
{

  P_SSD1331_DC(Bit_SET);
  P_SSD1331_CS(RESET);

  OledSpi_SendByte(color >> 8);
  OledSpi_SendByte(color);

  P_SSD1331_CS(Bit_SET);
}


//--------------------------------------------------------------
// Draws one pixel at x,y
//--------------------------------------------------------------
void OLED_DrawPixel(uint16_t color)
{
	  P_SSD1331_DC(Bit_SET);
	  P_SSD1331_CS(RESET);

	  OledSpi_SendByte(color >> 8);
	  OledSpi_SendByte(color);

	  P_SSD1331_CS(Bit_SET);
}


//--------------------------------------------------------------
// Fills the screen with one colour
//--------------------------------------------------------------


//--------------------------------------------------------------
// Draws a line in one colour
// from x1,y1 to x2,y2
//--------------------------------------------------------------


//--------------------------------------------------------------
// Draws a circle in one colour
// centre = x1,y1
// Radius = radius
// (Bresenham algorithm)
//--------------------------------------------------------------
void OLED_DrawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color)
{
  int16_t f=1-radius, ddF_x=0, ddF_y=-2*radius, x=0, y=radius;

  SSD1351_drawPixel(x0, y0 + radius, color);
  SSD1351_drawPixel(x0, y0 - radius, color);
  SSD1351_drawPixel(x0 + radius, y0, color);
  SSD1351_drawPixel(x0 - radius, y0, color);

  while(x < y)
  {
    if(f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x + 1;

    SSD1351_drawPixel(x0 + x, y0 + y, color);
    SSD1351_drawPixel(x0 - x, y0 + y, color);
    SSD1351_drawPixel(x0 + x, y0 - y, color);
    SSD1351_drawPixel(x0 - x, y0 - y, color);
    SSD1351_drawPixel(x0 + y, y0 + x, color);
    SSD1351_drawPixel(x0 - y, y0 + x, color);
    SSD1351_drawPixel(x0 + y, y0 - x, color);
    SSD1351_drawPixel(x0 - y, y0 - x, color);
  }
}


//--------------------------------------------------------------
// Draws one ASCII character of a font at x,y
// with foreground and background colour (font max 16 pixels wide)
// -> pass the font with the & operator
//--------------------------------------------------------------


void OLED_DrawChar(uint8_t x, uint8_t y, uint8_t ascii, OledFont *font, uint16_t vg, uint16_t bg)
{
  uint16_t xn,yn,maske;
  const uint16_t *wert;
  
  ascii -= 32;
  wert=&font->table[ascii * font->height];

      
  for(yn = 0; yn < font->height; yn++) {
    maske=0x80;
    OLED_SetCursor2Draw(x,yn+y);
    for(xn = 0; xn < font->width; xn++) {			
      if((wert[yn] & maske) == 0x00) {
        // draw pixel in background colour
        OLED_SetPixel(bg);
      }
      else {
        // draw pixel in foreground colour
        OLED_SetPixel(vg);
      }
      maske=(maske>>1);
    }
  }
}


void SSD1351_DrawChar(uint8_t x, uint8_t y, uint8_t ascii, OledFont *font, uint16_t vg, uint16_t bg)
{
  uint16_t xn,yn,maske;
  const uint16_t *wert;

  ascii -= 32;
  wert=&font->table[ascii * font->height];


  for(yn = 0; yn < font->height; yn++) {
    maske=0x80;
    SSD1351_goTo(x,yn+y);
    for(xn = 0; xn < font->width; xn++) {
      if((wert[yn] & maske) == 0x00) {
        // draw pixel in background colour
    	  SSD1351_SetPixel(bg);
      }
      else {
        // draw pixel in foreground colour
    	  SSD1351_SetPixel(vg);
      }
      maske=(maske>>1);
    }
  }
}



void SSD1351_DrawString(uint8_t x, uint8_t y,char *ptr, OledFont *font, uint16_t vg, uint16_t bg)
{
  uint16_t pos;

  pos=x;
  while (*ptr != 0) {
	  SSD1351_DrawChar(pos,y,*ptr,font,vg,bg);
    pos+=font->width;
    ptr++;
  }
}




//--------------------------------------------------------------
// Draws a string at x,y
// with foreground and background colour (font max 16 pixels wide)
// -> pass the font with the & operator
//--------------------------------------------------------------
void OLED_DrawString(uint8_t x, uint8_t y,char *ptr, OledFont *font, uint16_t vg, uint16_t bg)
{
  uint16_t pos;

  pos=x;
  while (*ptr != 0) {
    OLED_DrawChar(pos,y,*ptr,font,vg,bg);
    pos+=font->width;
    ptr++;
  }
}






//--------------------------------------------------------------
// Internal function
// Initialise all display IO pins
//--------------------------------------------------------------


void P_SSD1351_InitIO(void)
{
  /* The pins are configured by CubeMX (MX_GPIO_Init); set the idle levels. */
#if USE_SSD1331_RESET==1
  P_SSD1331_RES(Bit_RESET);
#endif
  P_SSD1331_CS(Bit_SET);
}



//--------------------------------------------------------------
// Internal function
// ChipSelect-Pin schalten
// wert [Bit_SET, Bit_RESET]
//--------------------------------------------------------------
void P_SSD1331_CS(BitAction wert)
{
  if (wert == Bit_RESET) {
    LCD_CS_GPIO_PORT->BSRR = (uint32_t)(LCD_CS_PIN) << 16U;
  }
  else {
    LCD_CS_GPIO_PORT->BSRR = (uint32_t)(LCD_CS_PIN);
  }
}


//--------------------------------------------------------------
// Internal function
// DC-Pin schalten
// wert [Bit_SET, Bit_RESET]
//--------------------------------------------------------------
void P_SSD1331_DC(BitAction wert)
{
  if (wert == Bit_RESET) {
    LCD_DC_GPIO_PORT->BSRR = (uint32_t)(LCD_DC_PIN) << 16U;
  }
  else {
    LCD_DC_GPIO_PORT->BSRR = (uint32_t)(LCD_DC_PIN);
  }
}


//--------------------------------------------------------------
// Internal function
// Reset-Pin schalten
// wert [Bit_SET, Bit_RESET]
//--------------------------------------------------------------
void P_SSD1331_RES(BitAction wert)
{
#if USE_SSD1331_RESET==1
  if (wert == Bit_RESET) {
    LCD_RESET_GPIO_PORT->BSRR = (uint32_t)(LCD_RESET_PIN) << 16U;
  }
  else {
    LCD_RESET_GPIO_PORT->BSRR = (uint32_t)(LCD_RESET_PIN);
  }
#endif
}



void writeCommand (uint8_t wert)
{
	  P_SSD1331_DC(Bit_RESET);

	  // chip select low
	  P_SSD1331_CS(Bit_RESET);

	  // send value
	  OledSpi_SendByte(wert);

	  // chip select high
	  P_SSD1331_CS(Bit_SET);
}

void writeData(uint8_t wert)
{
	  P_SSD1331_DC(Bit_SET);

	  // chip select low
	  P_SSD1331_CS(Bit_RESET);

	  // send value
	  OledSpi_SendByte(wert);

	  // chip select high
	  P_SSD1331_CS(Bit_SET);
}





//--------------------------------------------------------------
// Internal function
// initialises the display controller
//--------------------------------------------------------------


void SSD1351_init(void)
{

	  P_SSD1331_RES(Bit_RESET);
	  P_SSD1331_Delay(LCD_INIT_PAUSE);
	  P_SSD1331_RES(Bit_SET);
	  P_SSD1331_Delay(LCD_INIT_PAUSE);
	  P_SSD1331_RES(Bit_RESET);
	  P_SSD1331_Delay(LCD_INIT_PAUSE);
	  P_SSD1331_RES(Bit_SET);
	  P_SSD1331_Delay(LCD_INIT_PAUSE);

    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0x12);
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0xB1);

    writeCommand(SSD1351_CMD_DISPLAYOFF);  		// 0xAE

    writeCommand(SSD1351_CMD_CLOCKDIV);  		// 0xB3
    writeCommand(0xF1);  						// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)

    writeCommand(SSD1351_CMD_MUXRATIO);
    writeData(127);

    writeCommand(SSD1351_CMD_SETREMAP);
    writeData(0x74);

    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData(0x00);
    writeData(0x7F);
    writeCommand(SSD1351_CMD_SETROW);
    writeData(0x00);
    writeData(0x7F);

    writeCommand(SSD1351_CMD_STARTLINE); 		// 0xA1
    if (LCD_MAXX == 96) {
      writeData(96);
    } else {
      writeData(0);
    }


    writeCommand(SSD1351_CMD_DISPLAYOFFSET); 	// 0xA2
    writeData(0x0);

    writeCommand(SSD1351_CMD_SETGPIO);
    writeData(0x00);

    writeCommand(SSD1351_CMD_FUNCTIONSELECT);
    writeData(0x01); // internal (diode drop)
    //writeData(0x01); // external bias

//    writeCommand(SSSD1351_CMD_SETPHASELENGTH);
//    writeData(0x32);

    writeCommand(SSD1351_CMD_PRECHARGE);  		// 0xB1
    writeCommand(0x32);

    writeCommand(SSD1351_CMD_VCOMH);  			// 0xBE
    writeCommand(0x05);

    writeCommand(SSD1351_CMD_NORMALDISPLAY);  	// 0xA6

    writeCommand(SSD1351_CMD_CONTRASTABC);
    writeData(0xC8);
    writeData(0x80);
    writeData(0xC8);

    writeCommand(SSD1351_CMD_CONTRASTMASTER);
    writeData(0x0F);

    writeCommand(SSD1351_CMD_SETVSL );
    writeData(0xA0);
    writeData(0xB5);
    writeData(0x55);

    writeCommand(SSD1351_CMD_PRECHARGE2);
    writeData(0x01);

    writeCommand(SSD1351_CMD_DISPLAYON);

    //writeCommand(SSD1351_CMD_NORMALDISPLAY);

}

//--------------------------------------------------------------
// Internal function
// send a command to the display over SPI
//--------------------------------------------------------------


//--------------------------------------------------------------
// Internal function
// send data to the display over SPI
//--------------------------------------------------------------


//--------------------------------------------------------------
// Internal function
// short pause
//-------------------------------------------------------------- 
void P_SSD1331_Delay(volatile uint32_t nCount)
{
  volatile uint32_t index = 0;

  for(index = nCount; index != 0; index--);
}


//--------------------------------------------------------------
// Internal function
// converts a 16-bit colour to R,G,B
//--------------------------------------------------------------


//--------------------------------------------------------------
// Internal function
// Draws a line in one colour
// from x1,y1 to x2,y2
//--------------------------------------------------------------



void SSD1351_goTo(int x, int y) {
  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT)) return;

  // set x and y coordinate
  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(SSD1351WIDTH-1);

  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(SSD1351HEIGHT-1);

  writeCommand(SSD1351_CMD_WRITERAM);
}


void set_rotation(uint8_t x)
{
	rotation = (x & 3);

	  switch(rotation) {
	   case 0:
	   case 2:
		   TFT_WIDTH  = SSD1351WIDTH;
		   TFT_HEIGHT = SSD1351HEIGHT;
	    break;
	   case 1:
	   case 3:
		   TFT_WIDTH  = SSD1351HEIGHT;
		   TFT_HEIGHT = SSD1351WIDTH;
	    break;
	  }

}

uint16_t getRotation(void)
{
	return rotation;
}
uint16_t SSD1351_Color565(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t c;
  c = r >> 3;
  c <<= 6;
  c |= g >> 2;
  c <<= 5;
  c |= b >> 3;

  return c;
}

void SSD1351_fillScreen(uint16_t fillcolor) {
	SSD1351_fillRect(0, 0, SSD1351WIDTH, SSD1351HEIGHT, fillcolor);
}

void SSD1351_rawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fillcolor) {
  // Bounds check

	uint16_t i;

  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
    return;

  // Y bounds check
  if (y+h > SSD1351HEIGHT)
  {
    h = SSD1351HEIGHT - y - 1;
  }

  // X bounds check
  if (x+w > SSD1351WIDTH)
  {
    w = SSD1351WIDTH - x - 1;
  }


  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(x+w-1);
  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(y+h-1);
  // fill!
  writeCommand(SSD1351_CMD_WRITERAM);

  for ( i=0; i < w*h; i++) {
    writeData(fillcolor >> 8);
    writeData(fillcolor);
  }
}

void SSD1351_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fillcolor) {  //hw acceleration
  switch (getRotation()) {
  case 0:  // No rotation
	  SSD1351_rawFillRect(x, y, w, h, fillcolor);
    break;
  case 1:  // Rotated 90 degrees clockwise.
    swap(x, y);
    x = TFT_WIDTH - x - h;
    SSD1351_rawFillRect(x, y, h, w, fillcolor);
    break;
  case 2:  // Rotated 180 degrees clockwise.
    x = TFT_WIDTH - x - w;
    y = TFT_HEIGHT - y - h;
    SSD1351_rawFillRect(x, y, w, h, fillcolor);
    break;
  case 3:  // Rotated 270 degrees clockwise.
    swap(x, y);
    y = TFT_HEIGHT - y - w;
    SSD1351_rawFillRect(x, y, h, w, fillcolor);
    break;
  }
}


// Draw a horizontal line ignoring any screen rotation.

// Draw a vertical line ignoring any screen rotation.




void SSD1351_drawPixel(int16_t x, int16_t y, uint16_t color)
{
  // Transform x and y based on current rotation.
  switch (getRotation()) {
  // Case 0: No rotation
  case 1:  // Rotated 90 degrees clockwise.
    swap(x, y);
    x = TFT_WIDTH - x - 1;
    break;
  case 2:  // Rotated 180 degrees clockwise.
    x = TFT_WIDTH - x - 1;
    y = TFT_HEIGHT - y - 1;
    break;
  case 3:  // Rotated 270 degrees clockwise.
    swap(x, y);
    y = TFT_HEIGHT - y - 1;
    break;
  }

  // Bounds check.
  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT)) return;
  if ((x < 0) || (y < 0)) return;

  SSD1351_goTo(x, y);

  // setup for data
  P_SSD1331_DC(Bit_SET);
  P_SSD1331_CS(RESET);

  OledSpi_SendByte(color >> 8);
  OledSpi_SendByte(color);

  P_SSD1331_CS(Bit_SET);
}

void TFT_INIT(void)
{
	OLED_Init();
}




void TFT_DrawBMP( UG_S16 xp, UG_S16 yp, const EZ_BMP *bmp )
{
   UG_S16 x,y,xs;
   const UG_U16* p;
   UG_U16 tmp;

   if ( bmp->p == NULL ) return;

   /* Only support 16 BPP so far */
   if ( bmp->bpp == BMP_BPP_16 )
   {
      p = (const UG_U16*)bmp->p;
   }
   else
   {
      return;
   }

   xs = xp;
   for(y=0;y<bmp->height;y++)
   {
      xp = xs;
      for(x=0;x<bmp->width;x++)
      {
         tmp = *p++;



         UG_COLOR red_mask = 0xF800;
         UG_COLOR green_mask = 0x7E0;
         UG_COLOR blue_mask = 0x1F;

         UG_U8 r_val = (tmp & red_mask) >> 11;
         UG_U8 g_val = (tmp & green_mask) >> 5;
         UG_U8 b_val = (tmp & blue_mask);

         c = ((UG_COLOR)r_val<<11) | ((UG_COLOR)g_val<<5) | (UG_COLOR)b_val;



         SSD1351_drawPixel(xp++, yp, c);
      }
      yp++;
   }
}





// Vertical Scrolling (Full Screen)
//
// a: Scrolling Direction
// "0x00" (Upward)
// "0x01" (Downward)
// b: Set Numbers of Row Scroll per Step
// c: Set Time Interval between Each Scroll Step
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=









//--------------------------------------------------------------
// File     : oled_font.c
// Text drawing functions for the OLED
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "oled_font.h"
#include "SSD1351.h"



//--------------------------------------------------------------
// Draws one ASCII character of a monospaced font at x,y
// with foreground and background colour (font max 16 pixels wide)
// -> pass the font with the & operator
//--------------------------------------------------------------


void OledFont_DrawChar(uint16_t x, uint16_t y, uint8_t ascii, OledFont *font, uint16_t vg, uint16_t bg)
{
  uint16_t xn,yn,start_maske,maske;
  const uint16_t *wert;
  

  ascii -= 32;
  wert=&font->table[ascii * font->height];
  uint16_t LCD_DISPLAY_MODE = getRotation();
  if(LCD_DISPLAY_MODE==PORTRAIT) {
    start_maske=0x80;
    if(font->width>8) start_maske=0x8000;
    
    for(yn = 0; yn < font->height; yn++) {
      maske=start_maske;
      // set cursor
      OLED_SetCursor2Draw(x,yn+y);
      for(xn = 0; xn < font->width; xn++) {
        if((wert[yn] & maske) == 0x00) {
          // draw pixel in background colour
          OLED_DrawPixel(bg);
        }
        else {
          // draw pixel in foreground colour
          OLED_DrawPixel(vg);
        }
        maske=(maske>>1);
      }
    }
  }
  else {
    start_maske=0x80;
    if(font->width>8) start_maske=0x8000;

    for(yn = 0; yn < font->height; yn++) {
      maske=start_maske;
      // set cursor
      OLED_SetCursor2Draw(x+font->height-yn,y);
      for(xn = 0; xn < font->width; xn++) {
        if((wert[yn] & maske) == 0x00) {
          // draw pixel in background colour
          OLED_DrawPixel(bg);
        }
        else {
          // draw pixel in foreground colour
          OLED_DrawPixel(vg);
        }
        maske=(maske>>1);
      }
    }
  }
}


//--------------------------------------------------------------
// Draws a string in a monospaced font at x,y
// with foreground and background colour (font max 16 pixels wide)
// -> pass the font with the & operator
//--------------------------------------------------------------
void OledFont_DrawString(uint16_t x, uint16_t y,char *ptr, OledFont *font, uint16_t vg, uint16_t bg)
{
  uint16_t pos;
  uint16_t LCD_DISPLAY_MODE = getRotation();
  if(LCD_DISPLAY_MODE==PORTRAIT) {
    pos=x;
    while (*ptr != 0) {
      OledFont_DrawChar(pos,y,*ptr,font,vg,bg);
      pos+=font->width;
      ptr++;
    }
  }
  else {
    pos=y;
    while (*ptr != 0) {
      OledFont_DrawChar(x,pos,*ptr,font,vg,bg);
      pos+=font->width;
      ptr++;
    }
  }
}


//--------------------------------------------------------------
// Draws one ASCII character of a monospaced font at x,y
// with foreground and background colour (font max 32 pixels wide)
// -> pass the font with the & operator
//--------------------------------------------------------------
void OledFont_DrawChar32(uint16_t x, uint16_t y, uint8_t ascii, OledFont32 *font, uint16_t vg, uint16_t bg)
{
  uint16_t xn,yn;
  uint32_t start_maske,maske;
  const uint32_t *wert;


  ascii -= 32;
  wert=&font->table[ascii * font->height];
  uint16_t LCD_DISPLAY_MODE = getRotation();
  if(LCD_DISPLAY_MODE==PORTRAIT) {
    start_maske=0x80;
    if(font->width>8) start_maske=0x8000;
    if(font->width>16) start_maske=0x80000000;

    for(yn = 0; yn < font->height; yn++) {
      maske=start_maske;
      // set cursor
      OLED_SetCursor2Draw(x,yn+y);
      for(xn = 0; xn < font->width; xn++) {
        if((wert[yn] & maske) == 0x00) {
          // draw pixel in background colour
          OLED_DrawPixel(bg);
        }
        else {
          // draw pixel in foreground colour
          OLED_DrawPixel(vg);
        }
        maske=(maske>>1);
      }
    }
  }
  else {
    start_maske=0x80;
    if(font->width>8) start_maske=0x8000;
    if(font->width>16) start_maske=0x80000000;

    for(yn = 0; yn < font->height; yn++) {
      maske=start_maske;
      // set cursor
      OLED_SetCursor2Draw(x+font->height-yn,y);
      for(xn = 0; xn < font->width; xn++) {
        if((wert[yn] & maske) == 0x00) {
          // draw pixel in background colour
          OLED_DrawPixel(bg);
        }
        else {
          // draw pixel in foreground colour
          OLED_DrawPixel(vg);
        }
        maske=(maske>>1);
      }
    }
  }
}


//--------------------------------------------------------------
// Draws a string in a monospaced font at x,y
// with foreground and background colour (font max 32 pixels wide)
// -> pass the font with the & operator
//--------------------------------------------------------------
void OledFont_DrawString32(uint16_t x, uint16_t y,char *ptr, OledFont32 *font, uint16_t vg, uint16_t bg)
{
  uint16_t pos;
  uint16_t LCD_DISPLAY_MODE = getRotation();
  if(LCD_DISPLAY_MODE==PORTRAIT) {
    pos=x;
    while (*ptr != 0) {
      OledFont_DrawChar32(pos,y,*ptr,font,vg,bg);
      pos+=font->width;
      ptr++;
    }
  }
  else {
    pos=y;
    while (*ptr != 0) {
      OledFont_DrawChar32(x,pos,*ptr,font,vg,bg);
      pos+=font->width;
      ptr++;
    }
  }
}


//--------------------------------------------------------------
// Draws one ASCII character of a proportional font at x,y
// with foreground and background colour (font max 16 pixels wide)
// -> pass the font with the & operator
// return : width of the drawn character
//--------------------------------------------------------------
/*
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
*/



//--------------------------------------------------------------
// Draws a string in a proportional font at x,y
// with foreground and background colour (font max 16 pixels wide)
// -> pass the font with the & operator
//--------------------------------------------------------------


//--------------------------------------------------------------
// Draws one ASCII character of a proportional font at x,y
// with foreground and background colour (font max 32 pixels wide)
// -> pass the font with the & operator
// return : width of the drawn character
//--------------------------------------------------------------
uint16_t OledFont_DrawPChar32(uint16_t x, uint16_t y, uint8_t ascii, OledPFont32 *font, uint16_t vg, uint16_t bg)
{
  uint16_t xn,yn,width;
  uint32_t start_maske,maske;
  const uint32_t *wert;

  // limit check
  if(ascii<font->first_char) return 0;
  if(ascii>font->last_char) return 0;

  ascii -= font->first_char;
  wert=&font->table[ascii * (font->height+1)];
  width=wert[0];
  uint16_t LCD_DISPLAY_MODE = getRotation();
  if(LCD_DISPLAY_MODE==PORTRAIT) {
    start_maske=0x01;
    start_maske=start_maske<<(width-1);

    for(yn = 0; yn < font->height; yn++) {
      maske=start_maske;
      // set cursor
      OLED_SetCursor2Draw(x,yn+y);
      for(xn = 0; xn < width; xn++) {
        if((wert[yn+1] & maske) == 0x00) {
          // draw pixel in background colour
          OLED_DrawPixel(bg);
        }
        else {
          // draw pixel in foreground colour
          OLED_DrawPixel(vg);
        }
        maske=(maske>>1);
      }
    }
  }
  else {
    start_maske=0x01;
    start_maske=start_maske<<(width-1);

    for(yn = 0; yn < font->height; yn++) {
      maske=start_maske;
      // set cursor
      OLED_SetCursor2Draw(x+font->height-yn,y);
      for(xn = 0; xn < width; xn++) {
        if((wert[yn+1] & maske) == 0x00) {
          // draw pixel in background colour
          OLED_DrawPixel(bg);
        }
        else {
          // draw pixel in foreground colour
          OLED_DrawPixel(vg);
        }
        maske=(maske>>1);
      }
    }
  }

  return(width);
}


//--------------------------------------------------------------
// Draws a string in a proportional font at x,y
// with foreground and background colour (font max 32 pixels wide)
// -> pass the font with the & operator
//--------------------------------------------------------------
void OledFont_DrawPString32(uint16_t x, uint16_t y,char *ptr, OledPFont32 *font, uint16_t vg, uint16_t bg)
{
  uint16_t pos,width;

  uint16_t LCD_DISPLAY_MODE = getRotation();

  if(LCD_DISPLAY_MODE==PORTRAIT) {
    pos=x;
    while (*ptr != 0) {
      width=OledFont_DrawPChar32(pos,y,*ptr,font,vg,bg);
      pos+=width;
      ptr++;
    }
  }
  else {
    pos=y;
    while (*ptr != 0) {
      width=OledFont_DrawPChar32(x,pos,*ptr,font,vg,bg);
      pos+=width;
      ptr++;
    }
  }
}

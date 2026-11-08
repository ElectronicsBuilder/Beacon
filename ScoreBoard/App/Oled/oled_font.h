//--------------------------------------------------------------
// File     : oled_font.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __APP_FONT_H
#define __APP_FONT_H

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"



//--------------------------------------------------------------
// Monospaced font (max 16 pixels wide)
//-------------------------------------------------------------- 

typedef struct OledFont_t {
  const uint16_t *table; // glyph data table
  uint16_t width;        // character width (pixels)
  uint16_t height;       // character height (pixels)
}OledFont;


//--------------------------------------------------------------
// Monospaced font (max 32 pixels wide)
//--------------------------------------------------------------
typedef struct OledFont32_t {
  const uint32_t *table; // glyph data table
  uint16_t width;        // character width (pixels)
  uint16_t height;       // character height (pixels)
}OledFont32;


//--------------------------------------------------------------
// Proportional font (max 16 pixels wide)
//--------------------------------------------------------------
typedef struct OledPFont_t {
  const uint16_t *table; // glyph data table
  uint16_t height;       // character height (pixels)
  uint16_t first_char;   // first character (ASCII code)
  uint16_t last_char;    // last character (ASCII code)
}OledPFont;


//--------------------------------------------------------------
// Proportional font (max 32 pixels wide)
//--------------------------------------------------------------
typedef struct OledPFont32_t {
  const uint32_t *table; // glyph data table
  uint16_t height;       // character height (pixels)
  uint16_t first_char;   // first character (ASCII code)
  uint16_t last_char;    // last character (ASCII code)
}OledPFont32;


//--------------------------------------------------------------
// Fonts available to the application
// (comment out unused ones)
// each font used needs its C file from App/Fonts
// in the build (listed in CMakeLists.txt)
//--------------------------------------------------------------
// monospaced
extern OledFont Arial_7x10; 
extern OledFont Arial_8x13;
extern OledFont Arial_10x15;
extern OledFont Arial_11x18;
extern OledFont Arial_13x19;
extern OledFont Arial_14x22;
extern OledFont Arial_16x25;
extern OledFont32 Arial_18x27;
// proportional
extern OledPFont pArial_10;
extern OledPFont pArial_13;
extern OledPFont pArial_16;
extern OledPFont pComic_12;
extern OledPFont pComic_16;
extern OledPFont pTimes_12;
extern OledPFont pTimes_15;
extern OledPFont32 pArial_21;
extern OledPFont32 pComic_19;
extern OledPFont32 pTimes_18;



//--------------------------------------------------------------
// Public functions
//--------------------------------------------------------------

void OledFont_DrawChar(uint16_t x, uint16_t y, uint8_t ascii, OledFont *font, uint16_t vg, uint16_t bg);
void OledFont_DrawString(uint16_t x, uint16_t y,char *ptr, OledFont *font, uint16_t vg, uint16_t bg);
void OledFont_DrawChar32(uint16_t x, uint16_t y, uint8_t ascii, OledFont32 *font, uint16_t vg, uint16_t bg);
void OledFont_DrawString32(uint16_t x, uint16_t y,char *ptr, OledFont32 *font, uint16_t vg, uint16_t bg);
uint16_t OledFont_DrawPChar(uint16_t x, uint16_t y, uint8_t ascii, OledPFont *font, uint16_t vg, uint16_t bg);
void OledFont_DrawPString(uint16_t x, uint16_t y,char *ptr, OledPFont *font, uint16_t vg, uint16_t bg);
uint16_t OledFont_DrawPChar32(uint16_t x, uint16_t y, uint8_t ascii, OledPFont32 *font, uint16_t vg, uint16_t bg);
void OledFont_DrawPString32(uint16_t x, uint16_t y,char *ptr, OledPFont32 *font, uint16_t vg, uint16_t bg);


//--------------------------------------------------------------
#endif // __APP_FONT_H

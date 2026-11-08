





/* PC3 -> B2
 * PC4 -> G2
 * PC5 -> R2
 * PC6 -> B1
 * PC7 -> G1
 * PC8 -> R1
 * PC9 -> CLK
 * PC10 -> LAT
 * PC11 -> OE
 * PC12 -> A
 * PC13 -> B
 * PC14 -> C
 * PC15 -> D
*/

//IC2 EE SCL PB6
// I2C EE SDA PB9

//  PD8  : Reset    (Display Pin 6 = RST  / R)  ]
//  PD9     CS       (Display Pin 7 = OCS  / OC)
//   PD10  D/C      (Display Pin 5 = D/C  / DC)
//   PA5     SPI_CLK  (Display Pin 4 = SCK  / CK)
//   PB4      SPI_MISO (Display Pin 2 = MISO / SO)
//   PB5      SPI_MOSI (Display Pin 3 = MOSI / SI)


#ifndef _MATRIX_H_
#define _MATRIX_H_

#define ENABLE_MATRIX
#include "app_main.h"
#include "FONT.h"


#define OE      0//define if oe is high active=1 or low active=0
//max. tested resolutions: WIDTH 32*8, HEIGHT 16, recommended MODULES<=6 with HEIGHT=16 and WIDTH=32



//#define PRESCALER_SIZE  WIDTH*BRIGHTNESS
//#define LED_DATA_SIZE   PRESCALER_SIZE*(HEIGHT/2)

#define MODULES 4
#define WIDTH           32*MODULES//HUB08 type:64, HUB75 type:32
#define HEIGHT  32//HUB08 type:32, HUB75 type:16
#define BRIGHTNESS      7// 8 //in Bit, ex. 8 bit=2^8=256 steps

#define PRESCALER_SIZE  WIDTH*BRIGHTNESS
#define LED_DATA_SIZE   PRESCALER_SIZE*(HEIGHT)  //h/2


 //the pixel image in RGB
 struct color{
         uint8_t R;
         uint8_t G;
         uint8_t B;
 };
 struct color foreground_color;
 struct color background_color;
 struct color image[HEIGHT][WIDTH];//width height with each rgb



 void calcColorCorrection(void);

 /* Global LED brightness in percent (gamma corrected). Keys '+' / '-' adjust it. */
 #define MATRIX_BRIGHTNESS_DEFAULT  40
 #define MATRIX_BRIGHTNESS_MIN      5
 #define MATRIX_BRIGHTNESS_STEP     10
 void Matrix_SetBrightness(uint8_t percent);
 uint8_t Matrix_GetBrightness(void);
 void calcPrescaler(void);
 void APP_DMA2_Stream1_IRQ(void);
 void initLEDdata(void);
 void calculateLEDdata(void);
 void calculateTestLEDData(void);
 void calculateLEDString(char *s, int32_t x, int32_t y, sFONT font);
 void calculateLine(uint32_t x, uint32_t y, uint32_t width, uint32_t height, struct color line_color);
 void calculateRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, struct color frame_color, struct color fill_color);
 void calculateClearImage(void);
 void calculateScrollChar(char s, sFONT font);
 void calculateScrollText(char *s, sFONT font, uint8_t textlength, uint8_t y, int32_t *x);
 void updateLEDData(void);
 void invertForeBackgroundColor(void);
 void CHANGE_FONT(unsigned char FONT);
 void UG_DrawBMP(UG_S16 xp, UG_S16 yp, EZ_BMP *bmp);

 void calculatePixel(uint32_t x, uint32_t y,uint16_t color);


#endif







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


#define OE      0   // OE is active low

/*
 * Panel geometry: two chained 64x64 HUB75 panels = 128 columns.
 *
 * MATRIX_PANEL_ROWS 32: drive 32 rows (1/16 scan, address lines A-D only).
 *     This is how the original board is wired (HUB75 pin 8 / E tied to GND):
 *     image rows 0-15 appear on panel rows 0-15, rows 16-31 on panel rows 32-47.
 * MATRIX_PANEL_ROWS 64: full 64x64 (1/32 scan). Needs the E line (HUB75E pin 8)
 *     driven from PC2 instead of GND. Uses 6-bit grayscale (64 levels) so the
 *     refresh rate stays ~110 Hz with twice the scan rows.
 */
#ifndef MATRIX_PANEL_ROWS
#define MATRIX_PANEL_ROWS  32
#endif

#define WIDTH           128
#if MATRIX_PANEL_ROWS == 64
#define HEIGHT          64
#define BRIGHTNESS      6   // bit planes (6 bit = 64 grey levels)
#define MATRIX_E_BIT    (1u << 2)   // PC2 = row address E
#elif MATRIX_PANEL_ROWS == 32
#define HEIGHT          32
#define BRIGHTNESS      7   // bit planes (7 bit = 128 grey levels)
#define MATRIX_E_BIT    0u
#else
#error "MATRIX_PANEL_ROWS must be 32 or 64"
#endif
#define MATRIX_SCAN_ROWS (HEIGHT / 2)   // rows per scan (top half + bottom half lit together)

/* Words at the start of each shift window that still use the previous plane's
 * prescaler (DMA/timer pipeline offset of the original design). */
#define MATRIX_PSC_OFFSET 4

#define PRESCALER_SIZE  (WIDTH * BRIGHTNESS)
#define LED_DATA_SIZE   (PRESCALER_SIZE * MATRIX_SCAN_ROWS)


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
 void Matrix_Refresh(void);                 /* rebuild the DMA data if the image changed */
 extern volatile uint8_t matrix_dirty;
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

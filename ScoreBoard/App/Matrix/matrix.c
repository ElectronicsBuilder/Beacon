


#include "matrix.h"
#include "app_hal.h"
#include <strings.h>
#include <string.h>
#include "FONT.h"

/* Global variables declared in matrix.h (defined here once) */
struct color foreground_color;
struct color background_color;
struct color image[HEIGHT][WIDTH];//width height with each rgb

//#include "app_main.h"





 uint32_t height=0, led_data_size=0, width=0, prescaler_size=0;
 uint16_t led_data[LED_DATA_SIZE];//calculate led data array/size
 uint16_t prescaler[PRESCALER_SIZE];
 uint8_t color_correction[256];



#ifdef ENABLE_MATRIX

/*
 * Gamma and brightness.
 * Each colour channel is shown with BRIGHTNESS (7) bit planes (binary code
 * modulation), so a corrected value is 0..127. The table only applies gamma 2.0;
 * it keeps the full 7-bit range so grey levels are preserved at any brightness.
 *
 * Global brightness is done with the OE (output enable) line instead: plane k is
 * displayed while plane k+1 is shifted in, so switching OE off for the tail of
 * every 128-word shift window shortens all planes by the same fraction. The
 * refresh rate (~117 Hz) stays the same, which avoids low-brightness flicker.
 */
static uint8_t matrix_brightness = MATRIX_BRIGHTNESS_DEFAULT;   /* percent */
static uint32_t matrix_oe_on_words = (WIDTH * MATRIX_BRIGHTNESS_DEFAULT) / 100u;
volatile uint8_t matrix_dirty = 1;          /* image changed since the last calculateLEDdata() */

/* Rebuild statistics, readable over the debugger (ST-Link hot-plug). */
volatile uint32_t matrix_calc_cycles;       /* CPU cycles of the last rebuild */
volatile uint32_t matrix_calc_count;        /* number of rebuilds since boot */

void calcColorCorrection(void){
        const uint32_t max_level = (1u << BRIGHTNESS) - 1u;      /* 127 */
        for(uint32_t v=0; v<256; v++){
                uint32_t c = (v * v * max_level + (255u * 255u) / 2u) / (255u * 255u);  /* gamma 2.0 */
                if (v != 0u && c == 0u) {
                        c = 1;                                     /* keep dim colours visible */
                }
                color_correction[v] = (uint8_t)c;
        }
}

void Matrix_SetBrightness(uint8_t percent){
        if (percent > 100u) percent = 100u;
        if (percent < MATRIX_BRIGHTNESS_MIN) percent = MATRIX_BRIGHTNESS_MIN;
        matrix_brightness = percent;
        matrix_oe_on_words = (WIDTH * (uint32_t)percent + 50u) / 100u;
        if (matrix_oe_on_words == 0u) matrix_oe_on_words = 1u;
        matrix_dirty = 1;
}

/* Rebuild the DMA buffer only if something was drawn since the last rebuild. */
void Matrix_Refresh(void){
        if (matrix_dirty) {
                matrix_dirty = 0;
                calculateLEDdata();
        }
}

uint8_t Matrix_GetBrightness(void){
        return matrix_brightness;
}

void calcPrescaler(void){
        #if BRIGHTNESS>1
        uint32_t prescaler_width,prescaler_brightness;
        uint32_t prescaler_data[BRIGHTNESS];
        prescaler_data[0]=0;
        for(prescaler_width=0;prescaler_width<BRIGHTNESS-1;prescaler_width++){
                prescaler_data[prescaler_width+1]=1<<prescaler_width;
        }
        for(prescaler_width=0;prescaler_width<MATRIX_PSC_OFFSET;prescaler_width++){
                prescaler[prescaler_size++]=prescaler_data[BRIGHTNESS-2];
        }
        for(prescaler_width=0;prescaler_width<WIDTH;prescaler_width++){
                prescaler[prescaler_size++]=prescaler_data[BRIGHTNESS-1];
        }
        for(prescaler_brightness=0;prescaler_brightness<BRIGHTNESS-2;prescaler_brightness++){
                for(prescaler_width=0;prescaler_width<WIDTH;prescaler_width++){
                        prescaler[prescaler_size++]=prescaler_data[prescaler_brightness];
                }
        }
        for(prescaler_width=0;prescaler_width<WIDTH-MATRIX_PSC_OFFSET;prescaler_width++){
                prescaler[prescaler_size++]=prescaler_data[BRIGHTNESS-2];
        }
        #endif
}

/*
 * LED matrix refresh (HUB75, 4 panels of 32x32):
 *  - DMA2 Stream1 (TIM8_UP request) copies led_data[] to GPIOC->ODR on every
 *    TIM8 update, i.e. it bit-bangs R1/G1/B1/R2/G2/B2, A-D, LAT and OE.
 *  - DMA2 Stream2 (TIM8_CH1 request) loads TIM8->PSC from prescaler[] to stretch
 *    the display time of each bit plane (binary code modulation).
 *  - TIM8 CH4 on PC9 is the panel clock (PWM, active low).
 * TIM8, both DMA streams and the GPIOs are configured by CubeMX (ScoreBoard.ioc).
 * Note: the DMA writes the whole of GPIOC, so PC0 (USB power enable, active low)
 * is held low by the 0 in bit 0 of every led_data word.
 */

#define DMA_S1_FLAGS (DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1)
#define DMA_S2_FLAGS (DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2 | DMA_LIFCR_CDMEIF2 | DMA_LIFCR_CFEIF2)

/* Same sequence as the original SPL code (DMA_Cmd(DISABLE) without waiting),
 * which is known to work on this board. */
static void P_DMA_Stop(DMA_Stream_TypeDef *stream)
{
        stream->CR &= ~DMA_SxCR_EN;
}

static void P_DMA_Start(DMA_Stream_TypeDef *stream, uint32_t flags, uint32_t count)
{
        DMA2->LIFCR = flags;
        stream->NDTR = count;
        if (count != 0u) {
                stream->CR |= DMA_SxCR_EN;
        }
}

/* Called from DMA2_Stream1_IRQHandler (stm32f4xx_it.c): at the end of each
 * frame, re-align the prescaler sequence with the pixel data. */
void APP_DMA2_Stream1_IRQ(void){
        if (DMA2->LISR & DMA_LISR_TCIF1){
                DMA2->LIFCR = DMA_LIFCR_CTCIF1;
                P_DMA_Stop(DMA2_Stream2);
                P_DMA_Start(DMA2_Stream2, DMA_S2_FLAGS, prescaler_size);
        }
}

void initLEDdata(void){
        //pre calculate color correction
        calcColorCorrection();

        //pre calculate the prescaler values
        calcPrescaler();

        /* Stream1: led_data[] -> GPIOC->ODR */
        P_DMA_Stop(DMA2_Stream1);
        DMA2_Stream1->PAR  = (uint32_t)&GPIOC->ODR;
        DMA2_Stream1->M0AR = (uint32_t)&led_data[0];
        DMA2_Stream1->CR  |= DMA_SxCR_TCIE;
        P_DMA_Start(DMA2_Stream1, DMA_S1_FLAGS, led_data_size);

        /* Stream2: prescaler[] -> TIM8->PSC */
        P_DMA_Stop(DMA2_Stream2);
        DMA2_Stream2->PAR  = (uint32_t)&TIM8->PSC;
        DMA2_Stream2->M0AR = (uint32_t)&prescaler[0];
        P_DMA_Start(DMA2_Stream2, DMA_S2_FLAGS, prescaler_size);

        /* The frame-end resync must not wait behind the long application work, so
         * run it above the FreeRTOS-managed priorities (it calls no RTOS API).
         * CubeMX forces >= 5 in the .ioc for FreeRTOS projects, so set it here. */
        HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 1, 0);

        /* TIM8: DMA requests on CC1 and update, then start the clock output */
        __HAL_TIM_ENABLE_DMA(&htim8, TIM_DMA_CC1 | TIM_DMA_UPDATE);
        HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
}

#if OE != 0
#error "calculateLEDdata() is written for an active-low OE line"
#endif

/* GPIOC bits for a scan row: A..D on PC12..PC15, E on PC2 (64-row panels). */
static inline uint32_t P_RowAddress(uint32_t row){
        return ((row & 0xFu) << 12) | ((row & 0x10u) ? MATRIX_E_BIT : 0u);
}

/*
 * Build the DMA word stream from image[][] (same word layout as the original):
 * GPIOC bits 2=row address E (64-row panels), 3=B2-line, 4=G2-line, 5=R2,
 * 6=B1-line, 7=G1-line, 8=R1, 10=LAT, 11=OE (active low), 12..15=row address
 * A..D. Per scan row: BRIGHTNESS planes of
 * WIDTH words; the first plane is shifted with the previous row's address (that
 * row's last plane is still on display), the latch word switches to this row.
 * Bit mapping kept from the original ("RBG"): image .B -> bits 7/4, .G -> bits 6/3.
 */
void calculateLEDdata(void){
        /* gamma-corrected channel values of one row pair, computed once per row */
        static uint8_t top_r[WIDTH], top_g[WIDTH], top_b[WIDTH];
        static uint8_t bot_r[WIDTH], bot_g[WIDTH], bot_b[WIDTH];
        const uint32_t oe_on = matrix_oe_on_words;
        const uint32_t start = DWT->CYCCNT;
        uint32_t n = 0;

        for(uint32_t row=0; row<MATRIX_SCAN_ROWS; row++){
                for(uint32_t col=0; col<WIDTH; col++){
                        top_r[col] = color_correction[image[row][col].R];
                        top_g[col] = color_correction[image[row][col].G];
                        top_b[col] = color_correction[image[row][col].B];
                        bot_r[col] = color_correction[image[row+(HEIGHT/2)][col].R];
                        bot_g[col] = color_correction[image[row+(HEIGHT/2)][col].G];
                        bot_b[col] = color_correction[image[row+(HEIGHT/2)][col].B];
                }
                uint32_t addr = P_RowAddress((row + MATRIX_SCAN_ROWS - 1u) % MATRIX_SCAN_ROWS); /* previous row while plane 0 shifts in */
                for(uint32_t plane=0; plane<BRIGHTNESS; plane++){
                        for(uint32_t col=0; col<WIDTH; col++){
                                uint32_t bits = (((top_r[col] >> plane) & 1u) << 8) |
                                                (((top_b[col] >> plane) & 1u) << 7) |
                                                (((top_g[col] >> plane) & 1u) << 6) |
                                                (((bot_r[col] >> plane) & 1u) << 5) |
                                                (((bot_b[col] >> plane) & 1u) << 4) |
                                                (((bot_g[col] >> plane) & 1u) << 3);
                                if (col < WIDTH - 1u) {
                                        /* shift word: OE on (0) for the first oe_on words, off (0x800) after */
                                        led_data[n++] = (uint16_t)(addr | bits | ((col >= oe_on) ? 0x800u : 0u));
                                } else {
                                        /* last column: OE off + latch, switch to this row's address */
                                        addr = P_RowAddress(row);
                                        led_data[n++] = (uint16_t)(0xC00u | addr | bits);
                                }
                        }
                }
        }
        led_data_size = n;
        matrix_calc_cycles = DWT->CYCCNT - start;
        matrix_calc_count++;
}


/* Set one pixel; mark the image dirty only if the colour really changes. */
static inline void P_SetPixel(uint32_t y, uint32_t x, const struct color *c){
        struct color *p = &image[y][x];
        if (p->R != c->R || p->G != c->G || p->B != c->B) {
                *p = *c;
                matrix_dirty = 1;
        }
}

void calculateLEDString(char *s, int32_t x, int32_t y, sFONT font){
        int32_t i,l,j;
        while(*s){
                for(i=0;i<font.Height;i++){
                        l=font.table[((*s)-32)*(font.Height)+i];
                        for(j=0;j<font.Width;j++){
                                if(WIDTH>x+(font.Width-1-j) && x+(font.Width-1-j)>=0){//check width
                                        P_SetPixel((uint32_t)(y+i), (uint32_t)(x+(font.Width-1-j)),
                                                   ((l>>j)&0x1) ? &foreground_color : &background_color);
                                }
                        }
                }
                x+=font.Width;
                s++;
        }
}

/* Like calculateLEDString(), with every font pixel drawn as a scale x scale block
 * (e.g. Font8x8 at scale 2 = 16x16 characters). Clipped to the image. */
void calculateLEDStringScaled(const char *s, int32_t x, int32_t y, sFONT font, uint8_t scale){
        for (; *s; s++, x += (int32_t)font.Width * scale) {
                if (*s < 32 || *s > 126) continue;
                for (int32_t i = 0; i < (int32_t)font.Height; i++) {
                        uint16_t bits = font.table[(*s - 32) * font.Height + i];
                        for (int32_t j = 0; j < (int32_t)font.Width; j++) {
                                const struct color *c = ((bits >> j) & 1u) ? &foreground_color : &background_color;
                                int32_t px = x + ((int32_t)font.Width - 1 - j) * scale;
                                int32_t py = y + i * scale;
                                for (int32_t dy = 0; dy < scale; dy++) {
                                        for (int32_t dx = 0; dx < scale; dx++) {
                                                if (px + dx >= 0 && px + dx < WIDTH && py + dy >= 0 && py + dy < HEIGHT) {
                                                        P_SetPixel((uint32_t)(py + dy), (uint32_t)(px + dx), c);
                                                }
                                        }
                                }
                        }
                }
        }
}

void calculateLine(uint32_t x, uint32_t y, uint32_t width, uint32_t height, struct color line_color){
        matrix_dirty = 1;
        for(;x<width;x++){
                for(;y<height;y++){
                        image[y][x].R=line_color.R;
                        image[y][x].G=line_color.G;
                        image[y][x].B=line_color.B;
                }
        }
}





//EXPERIMENTAL
void DISPLAY_PIXEL(uint32_t x, uint32_t y)
{
    P_SetPixel(y, x, &foreground_color);
    //calculateLEDdata();
}




void UG_DrawBMP( UG_S16 xp, UG_S16 yp, const EZ_BMP *bmp )
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

         foreground_color.R= r_val;
         foreground_color.G= g_val;
         foreground_color.B= b_val;

         DISPLAY_PIXEL( xp++,  yp);

        // c = ((UG_COLOR)r_val<<11) | ((UG_COLOR)g_val<<5) | (UG_COLOR)b_val;



         //UB_Graphic2D_DrawPixelNormal(xp++, yp, c);
      }
      yp++;
   }
}



















void calculateRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, struct color frame_color, struct color fill_color){
        matrix_dirty = 1;
        //fill
        for(;x<width;x++){
                for(;y<height;y++){
                        image[y][x].R=fill_color.R;
                        image[y][x].G=fill_color.G;
                        image[y][x].B=fill_color.B;
                }
        }
        //lines
        calculateLine(x,y,width,y+1,frame_color);
        calculateLine(x,y,x+1,height,frame_color);
        calculateLine(width,y,width+1,height,frame_color);
        calculateLine(x,height,width,height+1,frame_color);
}

void calculateClearImage(void){
        matrix_dirty = 1;
        for(height=0;height<HEIGHT;height++){
                        for(width=0;width<WIDTH;width++){
                                image[height][width].R=0x0;
                                image[height][width].G=0x0;
                                image[height][width].B=0x0;
                        }
        }
}


void calculateScrollText(char *s, sFONT font, uint8_t textlength, uint8_t y, int32_t *x){
        if((*x)<textlength*-8){
                (*x)=WIDTH+8*textlength;
                for(;textlength>0;textlength--){
                        calculateLEDString(" ",textlength*8,y,font);
                }
                calculateLEDString(" ",textlength*-8,y,font);
        }
        calculateLEDString(s,(*x),y,font);
        (*x)--;
}

void updateLEDData(void){
        calculateLEDdata();
        /* same order as the original: timer off, streams off, reload, streams on, timer on */
        __HAL_TIM_DISABLE(&htim8);
        P_DMA_Stop(DMA2_Stream1);
        P_DMA_Stop(DMA2_Stream2);
        P_DMA_Start(DMA2_Stream1, DMA_S1_FLAGS, led_data_size);
        P_DMA_Start(DMA2_Stream2, DMA_S2_FLAGS, prescaler_size);
        __HAL_TIM_ENABLE(&htim8);
}


void CHANGE_FONT(unsigned char FONT)
{
	if(FONT == RED)
	{
		foreground_color.R=0xff;
	    foreground_color.G=0x00;
	    foreground_color.B=0x00;
	}
	else if(FONT == GREEN)
	{
		foreground_color.R=0x00;
	    foreground_color.G=0x00;
	    foreground_color.B=0xff;
	}
	else if(FONT == BLUE)
	{
		foreground_color.R=0x00;
	    foreground_color.G=0xff;
	    foreground_color.B=0x00;
	}
	else if(FONT == WHITE)
	{
		foreground_color.R=0xff;
	    foreground_color.G=0xff;
	    foreground_color.B=0xff;
	}
	else if(FONT == MTX_YELLOW)
	{
		foreground_color.R=0xff;
	    foreground_color.G=0x00;
	    foreground_color.B=0xff;      /* red + blue: shows pink (kept as in 2016) */
	}
	else if(FONT == SERVE_COLOR)
	{
		foreground_color.R=0xff;
	    foreground_color.G=0x93;
	    foreground_color.B=0x14;
	}

}

#endif


static const struct { const char *name; uint8_t r, g, b; } named_colors[MTX_C_COUNT] = {
  [MTX_C_WHITE]   = { "white",   0xff, 0xff, 0xff },
  [MTX_C_RED]     = { "red",     0xff, 0x00, 0x00 },
  [MTX_C_GREEN]   = { "green",   0x00, 0xff, 0x00 },
  [MTX_C_BLUE]    = { "blue",    0x00, 0x00, 0xff },
  [MTX_C_YELLOW]  = { "yellow",  0xff, 0xff, 0x00 },
  [MTX_C_CYAN]    = { "cyan",    0x00, 0xff, 0xff },
  [MTX_C_MAGENTA] = { "magenta", 0xff, 0x00, 0xff },
};

void Matrix_SetTextColor(MatrixColor c)
{
  if (c >= MTX_C_COUNT) c = MTX_C_WHITE;
  foreground_color.R = named_colors[c].r;
  foreground_color.G = named_colors[c].g;
  foreground_color.B = named_colors[c].b;
}

const char *Matrix_ColorName(MatrixColor c)
{
  return (c < MTX_C_COUNT) ? named_colors[c].name : "white";
}

int Matrix_ColorFind(const char *name)
{
  for (int i = 0; i < (int)MTX_C_COUNT; i++)
  {
    if (strcmp(name, named_colors[i].name) == 0) return i;
  }
  return -1;
}

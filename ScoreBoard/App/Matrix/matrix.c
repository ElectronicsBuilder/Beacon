


#include "matrix.h"
#include "app_hal.h"
#include <strings.h>
#include <string.h>
#include "FONT.h"
//#include "app_main.h"





 uint16_t abcd=0;
 uint32_t image_width_index=0,image_height_index=0,image_rgb_index=0;
 uint32_t pwm = 0, height=0, led_data_size=0, rgb_data=0, width=0,prescaler_size=0;
 uint16_t led_data[LED_DATA_SIZE];//calculate led data array/size
 uint16_t prescaler[PRESCALER_SIZE];
 uint8_t color_correction[256];



#ifdef ENABLE_MATRIX

/*
 * Brightness and gamma.
 * Each colour channel is shown with BRIGHTNESS (7) bit planes, so only bits 0..6
 * of a corrected value are displayed (0..127). The table maps an 8-bit colour
 * value (0..255) through gamma 2.0 and the global brightness to 0..127.
 * calculateLEDdata() uses the table, so a change shows on the next refresh.
 */
static uint8_t matrix_brightness = MATRIX_BRIGHTNESS_DEFAULT;   /* percent */

void calcColorCorrection(void){
        const uint32_t max_level = (1u << BRIGHTNESS) - 1u;      /* 127 */
        for(uint32_t v=0; v<256; v++){
                uint32_t g = (v * v + 127u) / 255u;                /* gamma 2.0, 0..255 */
                uint32_t c = (g * max_level * matrix_brightness + (255u * 100u) / 2u) / (255u * 100u);
                if (v != 0u && c == 0u && matrix_brightness != 0u) {
                        c = 1;                                     /* keep dim colours visible */
                }
                color_correction[v] = (uint8_t)c;
        }
}

void Matrix_SetBrightness(uint8_t percent){
        if (percent > 100u) percent = 100u;
        if (percent < MATRIX_BRIGHTNESS_MIN) percent = MATRIX_BRIGHTNESS_MIN;
        matrix_brightness = percent;
        calcColorCorrection();
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
        for(prescaler_width=0;prescaler_width<MODULES;prescaler_width++){
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
        for(prescaler_width=0;prescaler_width<WIDTH-MODULES;prescaler_width++){
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

        /* TIM8: DMA requests on CC1 and update, then start the clock output */
        __HAL_TIM_ENABLE_DMA(&htim8, TIM_DMA_CC1 | TIM_DMA_UPDATE);
        HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
}

void calculateLEDdata(void){
                rgb_data=0;height=0;led_data_size=0;pwm=0;
                //GPIO Pin configuration: 3=B2,4=G2,5=R2,6=B1,7=G1,8=R1,9=CLK,10=LAT,11=OE,12=A,13=B,14=C,15=D
                for(height=0;height<(HEIGHT/2);height++){
                        //check row line arrangement (for 16 we need 3bits, so only ABC is used, for 32 we use ABCD)
                        #if HEIGHT<=16
                        abcd=(((15+height)&0xf)<<12)&0x7000;//No D
                        #elif HEIGHT<=32
                        abcd=(((15+height)&0xf)<<12);



                        #endif
                        //pwm
                        for(pwm=0;pwm<BRIGHTNESS;pwm++){
                                for(width=0;width<(WIDTH-1);width++){
                                        //change image[height][width].R and so on to according led pixel color of your display (this is RBG)
                                        rgb_data=(((color_correction[image[height][width].R]>>pwm)&0x1)<<8)|(((color_correction[image[height][width].B]>>pwm)&0x1)<<7)|(((color_correction[image[height][width].G]>>pwm)&0x1)<<6)|(((color_correction[image[height+(HEIGHT/2)][width].R]>>pwm)&0x1)<<5)|(((color_correction[image[height+(HEIGHT/2)][width].B]>>pwm)&0x1)<<4)|(((color_correction[image[height+(HEIGHT/2)][width].G]>>pwm)&0x1)<<3);
                                        #if OE==1
                                        led_data[led_data_size++]=0x800|abcd|rgb_data;//ABCD,OE=1,LAT=0,CLK=0,R1=0,G1=0,B1=0,R2=0,G2=0,B2=0,NC,NC,NC
                                        #elif OE==0
                                        led_data[led_data_size++]=abcd|rgb_data;//0x800|abcd|e;//ABCD,OE=0,LAT=0,CLK=0,R1=0,G1=0,B1=0,R2=0,G2=0,B2=0,NC,NC,NC
                                        #endif
                                }
                                //last data line with latch
                                abcd=(height<<12);
                                //change image[height][width].R and so on to according led pixel color of your display (this is RBG)
                                rgb_data=(((color_correction[image[height][width].R]>>pwm)&0x1)<<8)|(((color_correction[image[height][width].B]>>pwm)&0x1)<<7)|(((color_correction[image[height][width].G]>>pwm)&0x1)<<6)|(((color_correction[image[height+(HEIGHT/2)][width].R]>>pwm)&0x1)<<5)|(((color_correction[image[height+(HEIGHT/2)][width].B]>>pwm)&0x1)<<4)|(((color_correction[image[height+(HEIGHT/2)][width].G]>>pwm)&0x1)<<3);
                                #if OE==1
                                led_data[led_data_size++]=0x400|abcd|rgb_data;//ABCD,OE=0,LAT=1,CLK=0,R1=0,G1=0,B1=0,R2=0,G2=0,B2=0,NC,NC,NC
                                #elif OE==0
                                led_data[led_data_size++]=0xC00|abcd|rgb_data;//ABCD,OE=1,LAT=1,CLK=0,R1=0,G1=0,B1=0,R2=0,G2=0,B2=0,NC,NC,NC
                                #endif
                        }
                }

}

void calculateTestLEDData(void){
        uint32_t height,width;
        static uint32_t offset=0;
        static uint8_t brightness=0;
        for(height=0;height<HEIGHT;height++){
                for(width=0;width<WIDTH;width++){
                        switch(offset){
                                default:
                                case 0:
                                        image[height][width].R=brightness;
                                        image[height][width].G=0x0;
                                        image[height][width].B=0x0;
                                        break;
                                case 1:
                                        image[height][width].R=0x0;
                                        image[height][width].G=brightness;
                                        image[height][width].B=0x0;
                                        break;
                                case 2:
                                        image[height][width].R=0x0;
                                        image[height][width].G=0x0;
                                        image[height][width].B=brightness;
                                        break;
                        }
                        offset=(offset+1)%3;
                }
        }
        brightness++;
}

void calculateLEDString(char *s, int32_t x, int32_t y, sFONT font){
        int32_t i,l,j;
        while(*s){
                for(i=0;i<font.Height;i++){
                        l=font.table[((*s)-32)*(font.Height)+i];
                        for(j=0;j<font.Width;j++){
                                if(WIDTH>x+(font.Width-1-j) && x+(font.Width-1-j)>=0){//check width
                                        if((l>>j)&0x1){
                                                image[y+i][x+(font.Width-1-j)].R=foreground_color.R;
                                                image[y+i][x+(font.Width-1-j)].G=foreground_color.G;
                                                image[y+i][x+(font.Width-1-j)].B=foreground_color.B;
                                        }else{
                                                image[y+i][x+(font.Width-1-j)].R=background_color.R;
                                                image[y+i][x+(font.Width-1-j)].G=background_color.G;
                                                image[y+i][x+(font.Width-1-j)].B=background_color.B;
                                        }
                                }
                        }
                }
                x+=font.Width;
                s++;
        }
}

void calculateLine(uint32_t x, uint32_t y, uint32_t width, uint32_t height, struct color line_color){
        for(;x<width;x++){
                for(;y<height;y++){
                        image[y][x].R=line_color.R;
                        image[y][x].G=line_color.G;
                        image[y][x].B=line_color.B;
                }
        }
}



void calculatePixel(uint32_t x, uint32_t y,uint16_t color)
				{
					CHANGE_FONT(color);

                        image[y][x].R=foreground_color.R;
                        image[y][x].G=foreground_color.G;
                        image[y][x].B=foreground_color.B;
                        calculateLEDdata();
                }


//EXPERIMENTAL
void DISPLAY_PIXEL(uint32_t x, uint32_t y)
{
    image[y][x].R=foreground_color.R;
    image[y][x].G=foreground_color.G;
    image[y][x].B=foreground_color.B;
    //calculateLEDdata();
}




void UG_DrawBMP( UG_S16 xp, UG_S16 yp, EZ_BMP* bmp )
{
   UG_S16 x,y,xs;
   UG_U8 r,g,b;
   UG_U16* p;
   UG_U16 tmp;
   UG_COLOR c;

   if ( bmp->p == NULL ) return;

   /* Only support 16 BPP so far */
   if ( bmp->bpp == BMP_BPP_16 )
   {
      p = (UG_U16*)bmp->p;
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
        for(height=0;height<HEIGHT;height++){
                        for(width=0;width<WIDTH;width++){
                                image[height][width].R=0x0;
                                image[height][width].G=0x0;
                                image[height][width].B=0x0;
                        }
        }
}

void calculateScrollChar(char s, sFONT font){
        char str[1];
        static uint32_t x=WIDTH-8;
        if(x<1){
                calculateLEDString(" ",0,0,font);
                x=WIDTH-8;
        }
        str[0]=s;
        calculateLEDString(str,x,0,font);
        x--;
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

void invertForeBackgroundColor(void){
        struct color temp_color;

        temp_color=foreground_color;
        foreground_color=background_color;
        background_color=temp_color;
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
	    foreground_color.B=0xff;      /* B field drives the green LEDs */
	}
	else if(FONT == SERVE_COLOR)
	{
		foreground_color.R=0xff;
	    foreground_color.G=0x93;
	    foreground_color.B=0x14;
	}

}

#endif


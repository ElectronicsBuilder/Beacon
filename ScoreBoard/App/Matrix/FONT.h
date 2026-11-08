

#ifndef __FONT_H
#define __FONT_H


    typedef struct _tFont
    {
      const uint16_t *table;
      const uint16_t Width;
      const uint16_t Height;

    } sFONT;


extern sFONT Font16x24;
extern sFONT Font12x12;
extern sFONT Font8x12;
extern sFONT Font8x8;

#endif


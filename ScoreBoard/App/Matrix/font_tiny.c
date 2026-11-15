/*
 * font_tiny.c - 3 x 5 pixel font. See font_tiny.h.
 */
#include "font_tiny.h"

/* 5 rows of 3 bits (MSB = left), packed as row0 << 12 | row1 << 9 | ... */
#define G(a, b, c, d, e) (uint16_t)(((a) << 12) | ((b) << 9) | ((c) << 6) | ((d) << 3) | (e))

static uint16_t P_Glyph(char ch)
{
  if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
  switch (ch)
  {
    case '0': return G(07, 05, 05, 05, 07);
    case '1': return G(02, 06, 02, 02, 07);
    case '2': return G(07, 01, 07, 04, 07);
    case '3': return G(07, 01, 07, 01, 07);
    case '4': return G(05, 05, 07, 01, 01);
    case '5': return G(07, 04, 07, 01, 07);
    case '6': return G(07, 04, 07, 05, 07);
    case '7': return G(07, 01, 01, 02, 02);
    case '8': return G(07, 05, 07, 05, 07);
    case '9': return G(07, 05, 07, 01, 07);
    case ':': return G(00, 02, 00, 02, 00);
    case '-': return G(00, 00, 07, 00, 00);
    case '+': return G(00, 02, 07, 02, 00);
    case '/': return G(01, 01, 02, 04, 04);
    case '.': return G(00, 00, 00, 00, 02);
    case ',': return G(00, 00, 00, 02, 04);
    case '!': return G(02, 02, 02, 00, 02);
    case '?': return G(07, 01, 03, 00, 02);
    case '\'': return G(02, 02, 00, 00, 00);
    case 'A': return G(02, 05, 07, 05, 05);
    case 'B': return G(06, 05, 06, 05, 06);
    case 'C': return G(03, 04, 04, 04, 03);
    case 'D': return G(06, 05, 05, 05, 06);
    case 'E': return G(07, 04, 06, 04, 07);
    case 'F': return G(07, 04, 06, 04, 04);
    case 'G': return G(03, 04, 05, 05, 03);
    case 'H': return G(05, 05, 07, 05, 05);
    case 'I': return G(07, 02, 02, 02, 07);
    case 'J': return G(01, 01, 01, 05, 02);
    case 'K': return G(05, 05, 06, 05, 05);
    case 'L': return G(04, 04, 04, 04, 07);
    case 'M': return G(05, 07, 07, 05, 05);
    case 'N': return G(06, 05, 05, 05, 05);
    case 'O': return G(02, 05, 05, 05, 02);
    case 'P': return G(06, 05, 06, 04, 04);
    case 'Q': return G(02, 05, 05, 06, 03);
    case 'R': return G(06, 05, 06, 05, 05);
    case 'S': return G(03, 04, 02, 01, 06);
    case 'T': return G(07, 02, 02, 02, 02);
    case 'U': return G(05, 05, 05, 05, 07);
    case 'V': return G(05, 05, 05, 05, 02);
    case 'W': return G(05, 05, 07, 07, 05);
    case 'X': return G(05, 05, 02, 05, 05);
    case 'Y': return G(05, 05, 02, 02, 02);
    case 'Z': return G(07, 01, 02, 04, 07);
    default:  return 0;                       /* space and unknown */
  }
}

static void P_Set(int32_t x, int32_t y, const struct color *c)
{
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  struct color *p = &image[y][x];
  if (p->R != c->R || p->G != c->G || p->B != c->B) { *p = *c; matrix_dirty = 1; }
}

int32_t TinyFont_Width(const char *s)
{
  int32_t n = 0;
  while (s[n]) n++;
  return n ? n * TINY_ADVANCE - 1 : 0;
}

int32_t TinyFont_Draw(const char *s, int32_t x, int32_t y)
{
  for (; *s; s++, x += TINY_ADVANCE)
  {
    uint16_t g = P_Glyph(*s);
    for (int32_t row = 0; row < TINY_HEIGHT; row++)
    {
      uint32_t bits = (g >> (3 * (TINY_HEIGHT - 1 - row))) & 7u;
      for (int32_t col = 0; col < 3; col++)
      {
        P_Set(x + col, y + row, (bits & (4u >> col)) ? &foreground_color : &background_color);
      }
      P_Set(x + 3, y + row, &background_color);          /* gap column */
    }
  }
  return x;
}

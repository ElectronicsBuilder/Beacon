/*
 * scenes.c - full-screen animations. See scenes.h.
 *
 * Drawn straight into the matrix image (matrix.h), then the overlay text
 * (white with a dark outline, so the animation shows around the letters).
 * Positions are fixed point 8.8 (256 = one pixel).
 *
 * Regions: with 32 driven rows the two lit bands of the panels (image rows
 * 0-15 and 16-31) are separated by 16 dark panel rows, so each band is its
 * own region: things are launched, fall and burst inside one band and never
 * jump across the gap. With 64 rows the whole height is one region.
 */
#include "scenes.h"
#include "matrix.h"
#include "layout.h"
#include "FONT.h"
#include "app_hal.h"
#include <string.h>

#define FRAME_MS      33u
#define FP            256
#define PARTICLES     160
#define ROCKETS       4

#if HEIGHT == 64
#define REGIONS       1
#define REGION_H      64
#define ROCKET_V_MIN  (FP * 6 / 5)      /* rises ~30..50 px */
#define ROCKET_V_MAX  (FP * 8 / 5)
#define SPARK_V_MIN   (FP / 5)
#define SPARK_V_MAX   (FP * 4 / 5)
#define SPARK_G       (FP / 64)
#define SPARK_LIFE    36
#define BURST_SPARKS  40
#define FIRE_COOL     6                 /* max cooling per row: flames ~2/3 of the height */
#else
#define REGIONS       2
#define REGION_H      16
#define ROCKET_V_MIN  (FP * 3 / 5)      /* rises ~7..12 px: bursts inside the band */
#define ROCKET_V_MAX  (FP * 4 / 5)
#define SPARK_V_MIN   (FP / 8)
#define SPARK_V_MAX   (FP * 2 / 5)
#define SPARK_G       (FP / 96)
#define SPARK_LIFE    20
#define BURST_SPARKS  24
#define FIRE_COOL     24
#endif
#define ROCKET_G      (FP / 40)

typedef struct {
  int32_t x, y, vx, vy;          /* 8.8, y relative to the region top */
  uint8_t r, g, b;
  uint8_t life;                  /* frames left, 0 = free */
  uint8_t region;
  uint8_t kind;                  /* scene specific (spark crackle, colour shade, ...) */
} Particle;

typedef struct {
  int32_t x, y, vy;              /* 8.8, y relative to the region top */
  uint8_t active;
  uint8_t wait;                  /* frames before launch */
  uint8_t region;
} Rocket;

static SceneId scene;
static uint32_t ms;
static uint32_t frame;
static uint32_t rng = 0x12345678u;
static Particle part[PARTICLES];
static Rocket rocket[ROCKETS];
static uint8_t heat[REGION_H + 2][WIDTH];   /* fire */
static int8_t sine[256];                    /* -127..127 */
static char text[TEXT_OVERLAY_MAX + 1];
static int32_t text_x;

/* ------------------------------------------------------------ helpers */

static uint32_t P_Rand(void)            /* xorshift32 */
{
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return rng;
}

static int32_t P_Range(int32_t lo, int32_t hi)
{
  return lo + (int32_t)(P_Rand() % (uint32_t)(hi - lo + 1));
}

/* sine table from a parabola per half wave (smooth enough for animation) */
static void P_SineInit(void)
{
  for (int t = 0; t < 128; t++)
  {
    int v = 4 * t * (128 - t) / 128;
    if (v > 127) v = 127;
    sine[t] = (int8_t)v;
    sine[t + 128] = (int8_t)-v;
  }
}

/* pixel in a region; anything outside the region is not drawn */
static void P_Pixel(int region, int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b)
{
  if (x < 0 || x >= WIDTH || y < 0 || y >= REGION_H) return;
  y += region * REGION_H;
  image[y][x].R = r;
  image[y][x].G = g;
  image[y][x].B = b;
}

static void P_Clear(void)
{
  memset(image, 0, sizeof image);
}

/* every pixel times keep/256 (trails) */
static void P_Fade(uint32_t keep)
{
  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      struct color *p = &image[y][x];
      p->R = (uint8_t)((p->R * keep) >> 8);
      p->G = (uint8_t)((p->G * keep) >> 8);
      p->B = (uint8_t)((p->B * keep) >> 8);
    }
  }
}

/* hue 0..255 -> bright RGB */
static void P_Hue(uint8_t h, uint8_t *r, uint8_t *g, uint8_t *b)
{
  uint8_t region = h / 43, rem = (uint8_t)((h - region * 43) * 6);
  switch (region)
  {
    case 0:  *r = 255; *g = rem; *b = 0; break;
    case 1:  *r = (uint8_t)(255 - rem); *g = 255; *b = 0; break;
    case 2:  *r = 0; *g = 255; *b = rem; break;
    case 3:  *r = 0; *g = (uint8_t)(255 - rem); *b = 255; break;
    case 4:  *r = rem; *g = 0; *b = 255; break;
    default: *r = 255; *g = 0; *b = (uint8_t)(255 - rem); break;
  }
}

static void P_RandomColor(uint8_t *r, uint8_t *g, uint8_t *b)
{
  P_Hue((uint8_t)P_Rand(), r, g, b);
}

/* small bitmap (rows of bits, MSB = left) in one colour */
static void P_Sprite(int region, int32_t x, int32_t y, const uint8_t *rows, int h, int w,
                     uint8_t r, uint8_t g, uint8_t b)
{
  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++)
      if (rows[i] & (0x80u >> j)) P_Pixel(region, x + j, y + i, r, g, b);
}

/* ------------------------------------------------------------ fireworks */

static void P_Burst(const Rocket *k)
{
  uint8_t r, g, b, r2, g2, b2;
  P_RandomColor(&r, &g, &b);
  P_RandomColor(&r2, &g2, &b2);
  uint32_t style = P_Rand() % 3u;              /* 0 ball, 1 ring, 2 two colours */
  int32_t ring_speed = P_Range(SPARK_V_MIN * 2, SPARK_V_MAX);
  int n = 0;
  for (int p = 0; p < PARTICLES && n < BURST_SPARKS; p++)
  {
    if (part[p].life != 0u) continue;
    uint32_t a = P_Rand();
    int32_t speed = (style == 1u) ? ring_speed : P_Range(SPARK_V_MIN, SPARK_V_MAX);
    int32_t dx = (int32_t)(a & 0xFF) - 128, dy = (int32_t)((a >> 8) & 0xFF) - 128;
    int32_t len = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) + 1;
    int second = (style == 2u) && (n & 1);
    part[p] = (Particle){ k->x, k->y, dx * speed / len, dy * speed / len,
                          second ? r2 : r, second ? g2 : g, second ? b2 : b,
                          (uint8_t)P_Range(SPARK_LIFE / 2, SPARK_LIFE), k->region,
                          (uint8_t)((P_Rand() & 3u) == 0u) };     /* 1 in 4 crackles */
    n++;
  }
}

static void P_Fireworks(void)
{
  P_Fade(170);
  for (int i = 0; i < ROCKETS; i++)
  {
    Rocket *k = &rocket[i];
    if (!k->active)
    {
      if (k->wait > 0u) { k->wait--; continue; }
      k->active = 1;
      k->region = (uint8_t)(P_Rand() % REGIONS);
      k->x = P_Range(12, WIDTH - 13) * FP;
      k->y = (REGION_H - 1) * FP;
      k->vy = -P_Range(ROCKET_V_MIN, ROCKET_V_MAX);
      continue;
    }
    k->y += k->vy;
    k->vy += ROCKET_G;
    P_Pixel(k->region, k->x / FP, k->y / FP, 255, 220, 150);
    P_Pixel(k->region, k->x / FP, k->y / FP + 1, 120, 70, 20);          /* trail */
    if (k->vy >= 0 || k->y < 3 * FP)                   /* apex: burst */
    {
      P_Burst(k);
      k->active = 0;
      k->wait = (uint8_t)P_Range(4, 26);
    }
  }
  for (int p = 0; p < PARTICLES; p++)
  {
    Particle *q = &part[p];
    if (q->life == 0u) continue;
    q->x += q->vx;
    q->y += q->vy;
    q->vx -= q->vx / 32;                               /* air drag */
    q->vy += SPARK_G;
    q->life--;
    uint32_t fade = q->life > 10u ? 256u : q->life * 25u;
    if (q->kind && q->life < 14u && (P_Rand() & 1u)) fade = 255u;      /* crackle: white flashes */
    uint8_t r = q->r, g = q->g, b = q->b;
    if (q->kind && q->life < 14u && fade == 255u) r = g = b = 255;
    P_Pixel(q->region, q->x / FP, q->y / FP,
            (uint8_t)(r * fade >> 8), (uint8_t)(g * fade >> 8), (uint8_t)(b * fade >> 8));
  }
}

/* ------------------------------------------------------------ stars, falling */

static void P_Stars(void)
{
  P_Clear();
  int count = REGION_H * REGIONS * 3 / 2;      /* 96 on 64 rows */
  for (int p = 0; p < count && p < PARTICLES; p++)
  {
    Particle *s = &part[p];
    if (s->life == 0u)
    {
      s->region = (uint8_t)(P_Rand() % REGIONS);
      s->x = P_Range(0, WIDTH - 1);
      s->y = P_Range(0, REGION_H - 1);
      s->vx = P_Range(2, 6);                  /* twinkle speed */
      s->life = (uint8_t)P_Range(1, 255);
      int warm = (P_Rand() & 3u) == 0u;       /* one in four is a warm star */
      s->r = 255;
      s->g = warm ? 200 : 255;
      s->b = warm ? 120 : 255;
    }
    s->life = (uint8_t)(s->life + s->vx);
    uint32_t phase = s->life;                 /* triangle 0..255..0 */
    uint32_t level = phase < 128u ? phase * 2u : (255u - phase) * 2u;
    P_Pixel(s->region, s->x, s->y, (uint8_t)(s->r * level >> 8), (uint8_t)(s->g * level >> 8), (uint8_t)(s->b * level >> 8));
    if (phase > 250u) s->life = 0;            /* fully dark: move somewhere else */
  }
  /* now and then a shooting star */
  Particle *m = &part[PARTICLES - 1];
  if (m->life == 0u && (P_Rand() % 90u) == 0u)
  {
    *m = (Particle){ P_Range(0, WIDTH / 2) * FP, P_Range(0, REGION_H / 3) * FP, FP * 2, FP / 2,
                     255, 255, 255, 20, (uint8_t)(P_Rand() % REGIONS), 0 };
  }
  if (m->life != 0u)
  {
    for (int t = 0; t < 6; t++)               /* head and fading tail */
    {
      uint8_t v = (uint8_t)(255 - t * 40);
      P_Pixel(m->region, (m->x - m->vx * t / 2) / FP, (m->y - m->vy * t / 2) / FP, v, v, v);
    }
    m->x += m->vx;
    m->y += m->vy;
    m->life--;
  }
}

static void P_Falling(int confetti)
{
  P_Clear();
  int count = confetti ? 110 : 90;
  for (int p = 0; p < count && p < PARTICLES; p++)
  {
    Particle *f = &part[p];
    if (f->life == 0u)
    {
      f->region = (uint8_t)(p % REGIONS);    /* spread evenly over the bands */
      f->x = P_Range(0, WIDTH - 1) * FP;
      f->y = (frame < 2u) ? P_Range(0, REGION_H - 1) * FP : -P_Range(1, 10) * FP;
      f->vy = confetti ? P_Range(FP / 5, FP / 2) : P_Range(FP / 10, FP / 4);
      f->vx = P_Range(-FP / 10, FP / 10);
      f->kind = (uint8_t)(P_Rand() & 1u);     /* snow: big / small flake, confetti: flip */
      if (confetti) P_RandomColor(&f->r, &f->g, &f->b);
      else { f->r = f->g = f->b = (uint8_t)P_Range(150, 255); }
      if (!confetti && f->kind) f->vy += FP / 10;   /* big flakes fall faster (closer) */
      f->life = 1;
    }
    f->y += f->vy;
    f->x += f->vx;
    if ((frame + (uint32_t)p) % 16u == 0u) f->vx = P_Range(-FP / 8, FP / 8);   /* drift */
    if (f->y >= REGION_H * FP) f->life = 0;   /* bottom of its band: start again at its top */
    int32_t x = f->x / FP, y = f->y / FP;
    P_Pixel(f->region, x, y, f->r, f->g, f->b);
    if (confetti)
    {
      if (((frame / 4u) + (uint32_t)p) & 1u) P_Pixel(f->region, x + 1, y, f->r / 2, f->g / 2, f->b / 2);   /* flipping */
      else                                    P_Pixel(f->region, x, y + 1, f->r / 2, f->g / 2, f->b / 2);
    }
    else if (f->kind)                          /* big flake: a small cross */
    {
      uint8_t d = (uint8_t)(f->r / 2);
      P_Pixel(f->region, x - 1, y, d, d, d);
      P_Pixel(f->region, x + 1, y, d, d, d);
      P_Pixel(f->region, x, y - 1, d, d, d);
      P_Pixel(f->region, x, y + 1, d, d, d);
    }
  }
}

/* ------------------------------------------------------------ colour fields */

static void P_Rainbow(void)
{
  for (int region = 0; region < REGIONS; region++)
  {
    for (int y = 0; y < REGION_H; y++)
    {
      for (int x = 0; x < WIDTH; x++)
      {
        uint8_t r, g, b;
        P_Hue((uint8_t)(x * 2 + y * 3 + frame * 4u), &r, &g, &b);
        P_Pixel(region, x, y, (uint8_t)(r >> 1), (uint8_t)(g >> 1), (uint8_t)(b >> 1));   /* half: text stays readable */
      }
    }
  }
}

static void P_Plasma(void)
{
  uint32_t t = frame * 3u;
  for (int region = 0; region < REGIONS; region++)
  {
    for (int y = 0; y < REGION_H; y++)
    {
      int sy = sine[(uint8_t)(y * 6 + t)];
      for (int x = 0; x < WIDTH; x++)
      {
        int v = sine[(uint8_t)(x * 4 + t)] + sy
              + sine[(uint8_t)((x + y) * 3 - t * 2)]
              + sine[(uint8_t)(((x - 64) * (x - 64) + (y - 32) * (y - 32)) / 16 + t)];
        uint8_t r, g, b;
        P_Hue((uint8_t)(v / 2 + (int)t), &r, &g, &b);
        P_Pixel(region, x, y, (uint8_t)(r * 3 / 5), (uint8_t)(g * 3 / 5), (uint8_t)(b * 3 / 5));
      }
    }
  }
}

/* classic fire: heat rises from a flickering bottom row and cools */
static void P_Fire(void)
{
  for (int x = 0; x < WIDTH; x++)
  {
    uint8_t h = (uint8_t)P_Range(150, 255);
    if ((P_Rand() % 9u) == 0u) h = 40;                  /* dark gaps: separate flames */
    heat[REGION_H + 1][x] = h;
    heat[REGION_H][x] = (uint8_t)((heat[REGION_H][x] + h) / 2);
  }
  for (int y = 0; y < REGION_H; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      int xl = x > 0 ? x - 1 : x, xr = x < WIDTH - 1 ? x + 1 : x;
      int v = (heat[y + 1][xl] + heat[y + 1][x] + heat[y + 1][xr] + heat[y + 2][x]) / 4;
      v -= P_Range(0, FIRE_COOL);
      heat[y][x] = (uint8_t)(v < 0 ? 0 : v);
    }
  }
  for (int region = 0; region < REGIONS; region++)
  {
    for (int y = 0; y < REGION_H; y++)
    {
      for (int x = 0; x < WIDTH; x++)
      {
        uint32_t h = heat[y][x];
        uint32_t r = h * 3u > 255u ? 255u : h * 3u;
        uint32_t g = h > 85u ? ((h - 85u) * 2u > 255u ? 255u : (h - 85u) * 2u) : 0u;
        uint32_t b = h > 200u ? (h - 200u) * 3u : 0u;
        P_Pixel(region, x, y, (uint8_t)r, (uint8_t)g, (uint8_t)b);
      }
    }
  }
}

/* ------------------------------------------------------------ particles */

/* green "code" streaks falling down the columns */
static void P_CodeRain(void)
{
  P_Fade(215);
  int count = WIDTH / 2;
  for (int p = 0; p < count && p < PARTICLES; p++)
  {
    Particle *d = &part[p];
    if (d->life == 0u)
    {
      d->region = (uint8_t)(P_Rand() % REGIONS);
      d->x = P_Range(0, WIDTH - 1);
      d->y = -P_Range(0, REGION_H * 2) * FP;
      d->vy = P_Range(FP / 4, FP);
      d->life = 1;
    }
    int32_t old = d->y / FP;
    d->y += d->vy;
    int32_t y = d->y / FP;
    for (int32_t t = old; t < y; t++) P_Pixel(d->region, d->x, t, 0, 190, 40);  /* trail: green */
    P_Pixel(d->region, d->x, y, 180, 255, 180);                               /* head: bright */
    if (y > REGION_H + 4) d->life = 0;
  }
  if ((P_Rand() & 3u) == 0u)                                      /* glitter */
    P_Pixel((int)(P_Rand() % REGIONS), P_Range(0, WIDTH - 1), P_Range(0, REGION_H - 1), 0, 120, 30);
}

/* flying through stars: points rush out from the centre */
static void P_Warp(void)
{
  P_Fade(110);
  int count = 90;
  for (int p = 0; p < count && p < PARTICLES; p++)
  {
    Particle *s = &part[p];
    if (s->life == 0u)
    {
      uint32_t a = P_Rand();
      int32_t dx = (int32_t)(a & 0xFF) - 128, dy = (int32_t)((a >> 8) & 0xFF) - 128;
      int32_t len = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) + 1;
      int32_t speed = P_Range(FP / 8, FP / 3);
      s->region = (uint8_t)(P_Rand() % REGIONS);
      s->x = (WIDTH / 2) * FP + dx * 2 * FP / len;
      s->y = (REGION_H / 2) * FP + dy * 2 * FP / len;
      s->vx = dx * speed / len * 2;                  /* panel is twice as wide */
      s->vy = dy * speed / len;
      s->life = 1;
      s->kind = (uint8_t)((P_Rand() & 7u) == 0u);    /* a few blue-ish stars */
    }
    s->x += s->vx;
    s->y += s->vy;
    s->vx += s->vx / 12;                             /* accelerate outwards */
    s->vy += s->vy / 12;
    int32_t x = s->x / FP, y = s->y / FP;
    if (x < 0 || x >= WIDTH || y < 0 || y >= REGION_H) { s->life = 0; continue; }
    int32_t ax = x - WIDTH / 2, ay = (y - REGION_H / 2) * 2;
    int32_t dist = (ax < 0 ? -ax : ax) + (ay < 0 ? -ay : ay);
    uint32_t v = (uint32_t)(dist * 5 > 255 ? 255 : dist * 5 + 20);
    P_Pixel(s->region, x, y, s->kind ? (uint8_t)(v / 2) : (uint8_t)v, s->kind ? (uint8_t)(v * 3 / 4) : (uint8_t)v, (uint8_t)v);
  }
}

static const uint8_t heart_bmp[6] = { 0x6C, 0xFE, 0xFE, 0x7C, 0x38, 0x10 };          /* 7 x 6 */
static const uint8_t balloon_bmp[6] = { 0x70, 0xF8, 0xF8, 0xF8, 0x70, 0x20 };        /* 5 x 6 with knot */

/* hearts or balloons floating up and swaying */
static void P_Floating(int balloons)
{
  P_Clear();
  int count = balloons ? 14 : 16;
  for (int p = 0; p < count && p < PARTICLES; p++)
  {
    Particle *f = &part[p];
    if (f->life == 0u)
    {
      f->region = (uint8_t)(p % REGIONS);
      f->x = P_Range(2, WIDTH - 9) * FP;
      f->y = ((frame < 2u) ? P_Range(0, REGION_H + 10) : REGION_H + P_Range(2, 24)) * FP;
      f->vy = -P_Range(FP / 8, FP / 3);
      f->kind = (uint8_t)P_Rand();                   /* sway phase */
      if (balloons) P_RandomColor(&f->r, &f->g, &f->b);
      else
      {
        static const uint8_t pinks[4][3] = { { 255, 0, 20 }, { 255, 40, 120 }, { 200, 0, 60 }, { 255, 90, 160 } };
        const uint8_t *c = pinks[P_Rand() & 3u];
        f->r = c[0]; f->g = c[1]; f->b = c[2];
      }
      f->life = 1;
    }
    f->y += f->vy;
    int32_t sway = sine[(uint8_t)(frame * 4u + f->kind)] / 50;   /* -2..2 px */
    int32_t x = f->x / FP + sway, y = f->y / FP;
    if (balloons)
    {
      P_Sprite(f->region, x, y, balloon_bmp, 6, 5, f->r, f->g, f->b);
      P_Pixel(f->region, x + 1, y + 1, 255, 255, 255);              /* shine */
      for (int s = 0; s < 6; s++)                                   /* string */
        P_Pixel(f->region, x + 2 + ((s + (int)(frame / 6u)) % 3 == 0 ? 1 : 0), y + 6 + s, 90, 90, 90);
    }
    else
    {
      P_Sprite(f->region, x, y, heart_bmp, 6, 7, f->r, f->g, f->b);
      P_Pixel(f->region, x + 1, y + 1, 255, 200, 220);              /* shine */
    }
    if (y < -14) f->life = 0;
  }
}

/* ------------------------------------------------------------ text over it */

/* text in white with a dark outline; the background stays visible */
static void P_OutlinedText(const char *s, int32_t x, int32_t y)
{
  const sFONT *font = &Font8x8;
  for (int pass = 0; pass < 2; pass++)
  {
    int32_t cx = x;
    for (const char *c = s; *c; c++, cx += font->Width)
    {
      if (*c < 32 || *c > 126 || cx >= WIDTH || cx + (int32_t)font->Width < -1) continue;
      for (int32_t i = 0; i < (int32_t)font->Height; i++)
      {
        uint16_t bits = font->table[(*c - 32) * font->Height + i];
        for (int32_t j = 0; j < (int32_t)font->Width; j++)
        {
          if (((bits >> j) & 1u) == 0u) continue;
          int32_t px = cx + ((int32_t)font->Width - 1 - j), py = y + i;
          if (pass == 0)
          {
            for (int32_t dy = -1; dy <= 1; dy++)
              for (int32_t dx = -1; dx <= 1; dx++)
                if (px + dx >= 0 && px + dx < WIDTH && py + dy >= 0 && py + dy < HEIGHT)
                  image[py + dy][px + dx] = (struct color){ 0, 0, 0 };
          }
          else if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT)
          {
            image[py][px] = (struct color){ 255, 255, 255 };
          }
        }
      }
    }
  }
}

static void P_Overlay(void)
{
  size_t n = strlen(text);
  if (n == 0u) return;
  int32_t y = LAYOUT_MSG_SINGLE_Y;
  int32_t len_px = (int32_t)n * 8;
  if (len_px <= WIDTH)
  {
    P_OutlinedText(text, LAYOUT_CENTER_X(n, 8), y);
  }
  else
  {
    P_OutlinedText(text, text_x, y);
    if (--text_x < -len_px) text_x = WIDTH;
  }
}

/* ------------------------------------------------------------ public */

void Scene_Start(SceneId id, const char *overlay)
{
  uint32_t seed = 0;
  HAL_RNG_GenerateRandomNumber(&hrng, &seed);
  if (seed != 0u) rng = seed;
  if (sine[64] == 0) P_SineInit();
  scene = id;
  ms = FRAME_MS;                       /* first frame right away */
  frame = 0;
  memset(part, 0, sizeof part);
  memset(rocket, 0, sizeof rocket);
  memset(heat, 0, sizeof heat);
  for (int i = 0; i < ROCKETS; i++) rocket[i].wait = (uint8_t)(i * 10);
  strncpy(text, overlay ? overlay : "", TEXT_OVERLAY_MAX);
  text[TEXT_OVERLAY_MAX] = '\0';
  text_x = WIDTH;
  P_Clear();
  matrix_dirty = 1;
}

void Scene_Tick(void)
{
  if (++ms < FRAME_MS) return;
  ms = 0;
  switch (scene)
  {
    case SCENE_FIREWORKS: P_Fireworks(); break;
    case SCENE_STARS:     P_Stars(); break;
    case SCENE_CONFETTI:  P_Falling(1); break;
    case SCENE_SNOW:      P_Falling(0); break;
    case SCENE_RAINBOW:   P_Rainbow(); break;
    case SCENE_CODERAIN:  P_CodeRain(); break;
    case SCENE_PLASMA:    P_Plasma(); break;
    case SCENE_FIRE:      P_Fire(); break;
    case SCENE_WARP:      P_Warp(); break;
    case SCENE_HEARTS:    P_Floating(0); break;
    case SCENE_BALLOONS:  P_Floating(1); break;
    default:              P_Clear(); break;
  }
  P_Overlay();
  frame++;
  matrix_dirty = 1;
}

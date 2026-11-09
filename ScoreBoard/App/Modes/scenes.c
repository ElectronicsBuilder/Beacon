/*
 * scenes.c - full-screen animations. See scenes.h.
 *
 * Drawn straight into the matrix image (matrix.h), then the overlay text.
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
#define PARTICLES     96
#define ROCKETS       3

#if HEIGHT == 64
#define REGIONS       1
#define REGION_H      64
#define ROCKET_V_MIN  (FP * 6 / 5)      /* rises ~30..50 px */
#define ROCKET_V_MAX  (FP * 8 / 5)
#define SPARK_V_MIN   (FP / 6)
#define SPARK_V_MAX   (FP * 3 / 4)
#define SPARK_G       (FP / 64)
#define SPARK_LIFE    32
#else
#define REGIONS       2
#define REGION_H      16
#define ROCKET_V_MIN  (FP * 3 / 5)      /* rises ~7..12 px: bursts inside the band */
#define ROCKET_V_MAX  (FP * 4 / 5)
#define SPARK_V_MIN   (FP / 8)
#define SPARK_V_MAX   (FP * 2 / 5)
#define SPARK_G       (FP / 96)
#define SPARK_LIFE    20
#endif
#define ROCKET_G      (FP / 40)

typedef struct {
  int32_t x, y, vx, vy;          /* 8.8, y relative to the region top */
  uint8_t r, g, b;
  uint8_t life;                  /* frames left, 0 = free */
  uint8_t region;
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

/* ------------------------------------------------------------ scenes */

static void P_Burst(const Rocket *k)
{
  uint8_t r, g, b;
  P_RandomColor(&r, &g, &b);
  int n = 0;
  for (int p = 0; p < PARTICLES && n < 32; p++)
  {
    if (part[p].life != 0u) continue;
    uint32_t a = P_Rand();
    int32_t speed = P_Range(SPARK_V_MIN, SPARK_V_MAX);
    int32_t dx = (int32_t)(a & 0xFF) - 128, dy = (int32_t)((a >> 8) & 0xFF) - 128;
    int32_t len = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) + 1;
    part[p] = (Particle){ k->x, k->y, dx * speed / len, dy * speed / len, r, g, b,
                          (uint8_t)P_Range(SPARK_LIFE / 2, SPARK_LIFE), k->region };
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
      k->x = P_Range(16, WIDTH - 17) * FP;
      k->y = (REGION_H - 1) * FP;
      k->vy = -P_Range(ROCKET_V_MIN, ROCKET_V_MAX);
      continue;
    }
    k->y += k->vy;
    k->vy += ROCKET_G;
    P_Pixel(k->region, k->x / FP, k->y / FP, 255, 200, 120);
    if (k->vy >= 0 || k->y < 2 * FP)                   /* apex: burst */
    {
      P_Burst(k);
      k->active = 0;
      k->wait = (uint8_t)P_Range(5, 30);
    }
  }
  for (int p = 0; p < PARTICLES; p++)
  {
    Particle *q = &part[p];
    if (q->life == 0u) continue;
    q->x += q->vx;
    q->y += q->vy;
    q->vy += SPARK_G;
    q->life--;
    uint32_t fade = q->life > 8u ? 256u : q->life * 32u;
    P_Pixel(q->region, q->x / FP, q->y / FP,
            (uint8_t)(q->r * fade >> 8), (uint8_t)(q->g * fade >> 8), (uint8_t)(q->b * fade >> 8));
  }
}

static void P_Stars(void)
{
  P_Clear();
  for (int p = 0; p < 60 && p < PARTICLES; p++)
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
}

static void P_Falling(int confetti)
{
  P_Clear();
  int count = confetti ? 80 : 60;
  for (int p = 0; p < count && p < PARTICLES; p++)
  {
    Particle *f = &part[p];
    if (f->life == 0u)
    {
      f->region = (uint8_t)(p % REGIONS);    /* spread evenly over the bands */
      f->x = P_Range(0, WIDTH - 1) * FP;
      f->y = (frame < 2u) ? P_Range(0, REGION_H - 1) * FP : -P_Range(1, 6) * FP;
      f->vy = confetti ? P_Range(FP / 6, FP * 2 / 5) : P_Range(FP / 12, FP / 5);
      f->vx = P_Range(-FP / 10, FP / 10);
      if (confetti) P_RandomColor(&f->r, &f->g, &f->b);
      else { f->r = f->g = f->b = (uint8_t)P_Range(160, 255); }
      f->life = 1;
    }
    f->y += f->vy;
    f->x += f->vx;
    if (!confetti && (frame + (uint32_t)p) % 16u == 0u) f->vx = P_Range(-FP / 10, FP / 10);   /* snow drifts */
    if (f->y >= REGION_H * FP) f->life = 0;   /* bottom of its band: start again at its top */
    P_Pixel(f->region, f->x / FP, f->y / FP, f->r, f->g, f->b);
    if (confetti) P_Pixel(f->region, f->x / FP + 1, f->y / FP, f->r, f->g, f->b);
  }
}

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

/* ------------------------------------------------------------ text over it */

static void P_Overlay(void)
{
  size_t n = strlen(text);
  if (n == 0u) return;
  Matrix_SetTextColor(MTX_C_WHITE);
  Matrix_SetBackColor(MTX_C_BLACK);
  int32_t y = LAYOUT_MSG_SINGLE_Y;
  int32_t len_px = (int32_t)n * 8;
  if (len_px <= WIDTH)
  {
    calculateLEDString(text, LAYOUT_CENTER_X(n, 8), y, Font8x8);
  }
  else
  {
    calculateLEDString(text, text_x, y, Font8x8);
    if (--text_x < -len_px) text_x = WIDTH;
  }
  CHANGE_FONT(BLUE);
}

/* ------------------------------------------------------------ public */

void Scene_Start(SceneId id, const char *overlay)
{
  uint32_t seed = 0;
  HAL_RNG_GenerateRandomNumber(&hrng, &seed);
  if (seed != 0u) rng = seed;
  scene = id;
  ms = 0;
  frame = 0;
  memset(part, 0, sizeof part);
  memset(rocket, 0, sizeof rocket);
  for (int i = 0; i < ROCKETS; i++) rocket[i].wait = (uint8_t)(i * 12);
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
    default:              break;
  }
  P_Overlay();
  frame++;
  matrix_dirty = 1;
}

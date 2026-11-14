/*
 * display_test.c - full LED matrix test. See display_test.h.
 * Draws straight into the matrix image (matrix.h).
 */
#include "display_test.h"
#include "matrix.h"
#include "layout.h"
#include "FONT.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

typedef enum { T_RED, T_GREEN, T_BLUE, T_WHITE, T_LINES, T_BARS, T_GRADIENT, T_CHECKER, T_ROWS, T_COLS, T_DONE } Step;

#define STEP_MS      2500u
#define ROW_STEP_MS  50u               /* row sweep: HEIGHT x 50 ms */
#define COL_STEP_MS  25u               /* column sweep: 128 x 25 ms */

static uint8_t active;
static Step step;
static uint32_t ms;
static int pos;                        /* row / column of the sweep */

static void P_Fill(uint8_t r, uint8_t g, uint8_t b)
{
  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      image[y][x].R = r; image[y][x].G = g; image[y][x].B = b;
    }
  }
  matrix_dirty = 1;
}

static void P_Px(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  image[y][x].R = r; image[y][x].G = g; image[y][x].B = b;
  matrix_dirty = 1;
}

static void P_Draw(void)
{
  static const uint8_t bar[8][3] = { {255,255,255}, {255,255,0}, {0,255,255}, {0,255,0},
                                     {255,0,255}, {255,0,0}, {0,0,255}, {0,0,0} };
  switch (step)
  {
    case T_RED:   P_Fill(255, 0, 0); break;
    case T_GREEN: P_Fill(0, 255, 0); break;
    case T_BLUE:  P_Fill(0, 0, 255); break;
    case T_WHITE: P_Fill(255, 255, 255); break;
    case T_LINES:
    {
      static const MatrixColor bg[4] = { MTX_C_RED, MTX_C_GREEN, MTX_C_BLUE, MTX_C_MAGENTA };
      P_Fill(0, 0, 0);
      int lines = HEIGHT / 16;
      for (int b = 0; b < lines; b++)
      {
        char text[12];
        snprintf(text, sizeof text, "LINE %d", b + 1);
        Matrix_SetBackColor(bg[b % 4]);
        Matrix_SetTextColor(MTX_C_WHITE);
        calculateLEDString("                ", 0, b * 16, Font8x8);
        calculateLEDString("                ", 0, b * 16 + 8, Font8x8);
        calculateLEDString(text, LAYOUT_CENTER_X(strlen(text), 8), b * 16 + 4, Font8x8);
      }
      Matrix_SetBackColor(MTX_C_BLACK);
      CHANGE_FONT(BLUE);
      break;
    }
    case T_BARS:
      for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
          P_Px(x, y, bar[x / 16][0], bar[x / 16][1], bar[x / 16][2]);
      break;
    case T_GRADIENT:                     /* top to bottom: red, green, blue, white ramps */
      for (int y = 0; y < HEIGHT; y++)
      {
        int band = y * 4 / HEIGHT;
        for (int x = 0; x < WIDTH; x++)
        {
          uint8_t v = (uint8_t)(x * 2);
          P_Px(x, y, (band == 0 || band == 3) ? v : 0, (band == 1 || band == 3) ? v : 0,
               (band == 2 || band == 3) ? v : 0);
        }
      }
      break;
    case T_CHECKER:
      for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
          P_Px(x, y, ((x + y) & 1) ? 255 : 0, ((x + y) & 1) ? 255 : 0, ((x + y) & 1) ? 255 : 0);
      break;
    case T_ROWS:
      P_Fill(0, 0, 0);
      for (int x = 0; x < WIDTH; x++) P_Px(x, pos, 255, 255, 255);
      break;
    case T_COLS:
      P_Fill(0, 0, 0);
      for (int y = 0; y < HEIGHT; y++) P_Px(pos, y, 255, 255, 255);
      break;
    default:
      break;
  }
}

void DisplayTest_Start(void)
{
  active = 1;
  step = T_RED;
  ms = 0;
  pos = 0;
  LOG_I("Display test: %dx%d, about 30 s (any key stops it)", WIDTH, HEIGHT);
  P_Draw();
}

int DisplayTest_Active(void) { return active; }

void DisplayTest_Stop(void)
{
  if (active)
  {
    active = 0;
    calculateClearImage();
  }
}

int DisplayTest_Tick(void)
{
  if (!active) return 0;
  ms++;
  if (step == T_ROWS || step == T_COLS)
  {
    uint32_t every = (step == T_ROWS) ? ROW_STEP_MS : COL_STEP_MS;
    int count = (step == T_ROWS) ? HEIGHT : WIDTH;
    if (ms < every) return 0;
    ms = 0;
    if (++pos < count)
    {
      P_Draw();
      return 0;
    }
    pos = 0;
    step = (Step)(step + 1);
  }
  else
  {
    if (ms < STEP_MS) return 0;
    ms = 0;
    step = (Step)(step + 1);
  }
  if (step >= T_DONE)
  {
    DisplayTest_Stop();
    LOG_I("Display test done");
    return 1;
  }
  P_Draw();
  return 0;
}

/*
 * display_test.h - full LED matrix test ("test" command, GUI button).
 *
 * About 30 s, then the shown mode comes back (any key / ESC stops it):
 * red, green, blue, white; the 4 lines labelled in different colours;
 * colour bars; a brightness gradient; a checkerboard; a line moving over
 * every row, then every column (dead rows or columns are easy to see).
 */
#ifndef APP_DISPLAY_TEST_H
#define APP_DISPLAY_TEST_H

void DisplayTest_Start(void);
int  DisplayTest_Active(void);
void DisplayTest_Stop(void);
int  DisplayTest_Tick(void);       /* every 1 ms; returns 1 when it has just ended */

#endif

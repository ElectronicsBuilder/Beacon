/*
 * Host unit tests for hardware-independent firmware logic.
 * Build and run: tests/host/run_tests.sh (uses the PC's gcc).
 */
#include <stdio.h>
#include <string.h>
#include "clock_calc.h"
#include "cli_parse.h"

static int failures, checks;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
  printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static ClockTime base(void)
{
  ClockTime t = { 2026, 11, 8, 12, 0, 0, 7 };
  return t;
}

static void test_clock(void)
{
  ClockTime t;

  /* days per month, leap years */
  CHECK(ClockCalc_DaysInMonth(2024, 2) == 29);
  CHECK(ClockCalc_DaysInMonth(2026, 2) == 28);
  CHECK(ClockCalc_DaysInMonth(2026, 4) == 30);
  CHECK(ClockCalc_DaysInMonth(2026, 12) == 31);

  /* weekday: 2026-11-08 is a Sunday, 2000-01-01 a Saturday, 2024-02-29 a Thursday */
  CHECK(ClockCalc_Weekday(2026, 11, 8) == 7);
  CHECK(ClockCalc_Weekday(2000, 1, 1) == 6);
  CHECK(ClockCalc_Weekday(2024, 2, 29) == 4);

  /* full date and time */
  t = base();
  CHECK(ClockCalc_Parse("2027-01-31", "23:59:58", &t) == 0);
  CHECK(t.year == 2027 && t.month == 1 && t.day == 31);
  CHECK(t.hour == 23 && t.minute == 59 && t.second == 58);

  /* time only keeps the date; seconds optional */
  t = base();
  CHECK(ClockCalc_Parse(NULL, "07:05", &t) == 0);
  CHECK(t.year == 2026 && t.month == 11 && t.day == 8 && t.hour == 7 && t.minute == 5 && t.second == 0);

  /* date only keeps the time */
  t = base();
  CHECK(ClockCalc_Parse("2028-02-29", NULL, &t) == 0);
  CHECK(t.hour == 12 && t.weekday == 2);

  /* invalid input is rejected */
  t = base(); CHECK(ClockCalc_Parse("2026-02-29", NULL, &t) != 0);   /* not a leap year */
  t = base(); CHECK(ClockCalc_Parse("2026-13-01", NULL, &t) != 0);
  t = base(); CHECK(ClockCalc_Parse("1999-12-31", NULL, &t) != 0);   /* RTC range 2000..2099 */
  t = base(); CHECK(ClockCalc_Parse(NULL, "24:00", &t) != 0);
  t = base(); CHECK(ClockCalc_Parse(NULL, "12:60", &t) != 0);
  t = base(); CHECK(ClockCalc_Parse(NULL, "12", &t) != 0);
  t = base(); CHECK(ClockCalc_Parse("2026-11-08x", NULL, &t) != 0);  /* trailing garbage */
  t = base(); CHECK(ClockCalc_Parse(NULL, "12:00:00z", &t) != 0);

  char text[24];
  t = base();
  Clock_Format(&t, text, sizeof text);
  CHECK(strcmp(text, "2026-11-08 12:00:00") == 0);
}

static void test_tokenizer(void)
{
  char *argv[8];
  char line1[] = "  time   2026-11-08 12:00 ";
  CHECK(Cli_Tokenize(line1, argv, 8) == 3);
  CHECK(strcmp(argv[0], "time") == 0 && strcmp(argv[2], "12:00") == 0);

  char line2[] = "msg add \"Hello world\" \"say \\\"hi\\\"\" red";
  CHECK(Cli_Tokenize(line2, argv, 8) == 5);
  CHECK(strcmp(argv[2], "Hello world") == 0);
  CHECK(strcmp(argv[3], "say \"hi\"") == 0);
  CHECK(strcmp(argv[4], "red") == 0);

  char line3[] = "";
  CHECK(Cli_Tokenize(line3, argv, 8) == 0);

  char line4[] = "a b c d e";
  CHECK(Cli_Tokenize(line4, argv, 3) == 3);          /* never more than max_args */

  char line5[] = "msg add \"unterminated";
  CHECK(Cli_Tokenize(line5, argv, 8) == 3);
  CHECK(strcmp(argv[2], "unterminated") == 0);
}

int main(void)
{
  test_clock();
  test_tokenizer();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}

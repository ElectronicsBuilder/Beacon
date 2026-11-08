/*
 * Host unit tests for hardware-independent firmware logic.
 * Build and run: tests/host/run_tests.sh (uses the PC's gcc).
 */
#include <stdio.h>
#include <string.h>
#include "clock_calc.h"
#include "cli_parse.h"
#include "rtc_trim_calc.h"
#include "time_entry.h"
#include "keys.h"
#include "crc16.h"

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

/* RTC rate after trimming, as a fraction error in ppm (double maths on the PC) */
static double trim_error_ppm(double f_lsi, const RtcTrim *t)
{
  double x = (double)t->calm - 512.0 * t->calp;
  double hz = f_lsi * 1048576.0 / (1048576.0 + x) / ((t->prediv_a + 1.0) * (t->prediv_s + 1.0));
  return (hz - 1.0) * 1e6;
}

static void test_rtc_trim(void)
{
  const uint32_t timer_hz = 84000000u, periods = 512u;
  const double lsi[] = { 17000.0, 31234.567, 32000.0, 32768.0, 32003.3, 41999.9, 47000.0 };
  RtcTrim t;

  for (unsigned i = 0; i < sizeof lsi / sizeof lsi[0]; i++)
  {
    uint32_t counts = (uint32_t)(timer_hz * (double)periods / lsi[i] + 0.5);
    double f = timer_hz * (double)periods / counts;           /* what the board can see */
    CHECK(RtcTrim_Calc(timer_hz, periods, counts, 0, &t) == 0);
    double err = trim_error_ppm(f, &t);
    CHECK(err > -1.0 && err < 1.0);
    CHECK(t.prediv_a == 3 && t.calm <= 511 && t.calp <= 1);
    CHECK(t.lsi_mhz / 1000u == (uint32_t)f);
  }

  /* small LSI drift keeps the prescaler (no sub-second restart) */
  uint32_t c1 = (uint32_t)(timer_hz * 512.0 / 32000.0);
  CHECK(RtcTrim_Calc(timer_hz, periods, c1, 0, &t) == 0);
  uint16_t s = t.prediv_s;
  uint32_t c2 = (uint32_t)(timer_hz * 512.0 / (32000.0 * 1.0003));   /* +300 ppm */
  CHECK(RtcTrim_Calc(timer_hz, periods, c2, s, &t) == 0);
  CHECK(t.prediv_s == s);
  CHECK(trim_error_ppm(timer_hz * 512.0 / c2, &t) < 1.0 && trim_error_ppm(timer_hz * 512.0 / c2, &t) > -1.0);
  /* a large drift picks a new prescaler */
  uint32_t c3 = (uint32_t)(timer_hz * 512.0 / (32000.0 * 1.01));
  CHECK(RtcTrim_Calc(timer_hz, periods, c3, s, &t) == 0);
  CHECK(t.prediv_s != s);

  /* nonsense measurements are rejected */
  CHECK(RtcTrim_Calc(timer_hz, periods, 0, 0, &t) != 0);
  CHECK(RtcTrim_Calc(timer_hz, periods, timer_hz, 0, &t) != 0);        /* 512 Hz */
  CHECK(RtcTrim_Calc(timer_hz, periods, 1000u, 0, &t) != 0);           /* 43 MHz */
}

static void type_keys(TimeEntry *e, const char *keys)
{
  while (*keys) TimeEntry_Key(e, (uint8_t)*keys++);
}

static void test_time_entry(void)
{
  TimeEntry e;
  ClockTime t = base();

  TimeEntry_Start(&e, &t);
  CHECK(strcmp(e.text, "2026-11-08 12:00") == 0 && e.pos == 0);

  /* typing all digits skips the separators */
  type_keys(&e, "202712312359");
  CHECK(strcmp(e.text, "2027-12-31 23:59") == 0);
  CHECK(e.pos == 15);                            /* stays on the last digit */
  CHECK(TimeEntry_Key(&e, KB_ENTER) == TE_DONE);
  CHECK(TimeEntry_Get(&e, &t) == 0);
  CHECK(t.year == 2027 && t.month == 12 && t.day == 31 && t.hour == 23 && t.minute == 59 && t.second == 0);

  /* cursor movement */
  TimeEntry_Start(&e, &t);
  TimeEntry_Key(&e, KB_LEFT);
  CHECK(e.pos == 15);                            /* wraps to the end */
  TimeEntry_Key(&e, KB_RIGHT);
  CHECK(e.pos == 0);                             /* and back to the start */
  for (int i = 0; i < 4; i++) TimeEntry_Key(&e, KB_RIGHT);
  CHECK(e.pos == 5);                             /* jumped over '-' */
  TimeEntry_Key(&e, KB_BACK);
  CHECK(e.pos == 3);
  type_keys(&e, "8");
  CHECK(strncmp(e.text, "2028", 4) == 0 && e.pos == 5);

  /* Up/Down step the field under the cursor, wrapping in its range */
  t = (ClockTime){ 2028, 2, 28, 23, 59, 0, 0 };
  TimeEntry_Start(&e, &t);
  e.pos = 9;  TimeEntry_Key(&e, KB_UP);          /* day 28 -> 29 (leap year) */
  CHECK(strncmp(e.text + 8, "29", 2) == 0);
  TimeEntry_Key(&e, KB_UP);                      /* 29 -> 1 */
  CHECK(strncmp(e.text + 8, "01", 2) == 0);
  TimeEntry_Key(&e, KB_DOWN);                    /* 1 -> 29 */
  CHECK(strncmp(e.text + 8, "29", 2) == 0);
  e.pos = 15; TimeEntry_Key(&e, KB_UP);          /* minute 59 -> 00 */
  CHECK(strcmp(e.text + 11, "23:00") == 0);
  e.pos = 11; TimeEntry_Key(&e, KB_UP);          /* hour 23 -> 00 */
  CHECK(strcmp(e.text + 11, "00:00") == 0);
  e.pos = 2;  TimeEntry_Key(&e, KB_DOWN);        /* year 2028 -> 2027 */
  CHECK(strncmp(e.text, "2027", 4) == 0);
  e.pos = 5;  TimeEntry_Key(&e, KB_DOWN); TimeEntry_Key(&e, KB_DOWN);   /* month 2 -> 1 -> 12 */
  CHECK(strncmp(e.text + 5, "12", 2) == 0);

  /* invalid date is reported, ESC cancels */
  TimeEntry_Start(&e, &t);
  type_keys(&e, "20270230");                     /* 30 February */
  CHECK(TimeEntry_Get(&e, &t) != 0);
  CHECK(TimeEntry_Key(&e, KB_ESC) == TE_CANCEL);
  CHECK(TimeEntry_Key(&e, 'x') == TE_EDITING);   /* other keys are ignored */
}

static void test_seconds_and_durations(void)
{
  ClockTime t = { 2000, 1, 1, 0, 0, 0, 6 }, u;
  CHECK(ClockCalc_ToSeconds(&t) == 0u);
  t = (ClockTime){ 2000, 3, 1, 0, 0, 0, 0 };
  CHECK(ClockCalc_ToSeconds(&t) == 60u * 86400u);             /* 31 + 29 days */
  t = (ClockTime){ 2026, 11, 8, 4, 45, 1, 0 };
  uint32_t s = ClockCalc_ToSeconds(&t);
  CHECK(s == 847428301u);                                     /* 2026-11-08 04:45:01 */
  ClockCalc_FromSeconds(s, &u);
  CHECK(u.year == 2026 && u.month == 11 && u.day == 8 && u.hour == 4 && u.minute == 45 && u.second == 1 && u.weekday == 7);

  /* round trip over every day boundary 2000..2099 */
  int bad = 0;
  for (uint32_t d = 0; d < 36524u; d += 1u)
  {
    ClockCalc_FromSeconds(d * 86400u + 86399u, &u);
    if (!ClockCalc_IsValid(&u) || ClockCalc_ToSeconds(&u) != d * 86400u + 86399u) bad++;
  }
  CHECK(bad == 0);

  /* leap day 2028-02-29 to 2028-03-01 */
  ClockTime a = { 2028, 2, 29, 23, 59, 59, 0 }, b = { 2028, 3, 1, 0, 0, 0, 0 };
  CHECK(ClockCalc_ToSeconds(&b) - ClockCalc_ToSeconds(&a) == 1u);

  char hms[12];
  CHECK(ClockCalc_FormatDuration(0, hms, sizeof hms) == 0 && strcmp(hms, "00:00:00") == 0);
  CHECK(ClockCalc_FormatDuration(3u * 86400u + 3661u, hms, sizeof hms) == 3 && strcmp(hms, "01:01:01") == 0);
  CHECK(ClockCalc_FormatDuration(86399u, hms, sizeof hms) == 0 && strcmp(hms, "23:59:59") == 0);

  uint32_t secs;
  CHECK(ClockCalc_ParseDuration("1h30m", &secs) == 0 && secs == 5400u);
  CHECK(ClockCalc_ParseDuration("2d", &secs) == 0 && secs == 172800u);
  CHECK(ClockCalc_ParseDuration("45s", &secs) == 0 && secs == 45u);
  CHECK(ClockCalc_ParseDuration("90", &secs) == 0 && secs == 5400u);
  CHECK(ClockCalc_ParseDuration("1d2h3m4s", &secs) == 0 && secs == 93784u);
  CHECK(ClockCalc_ParseDuration("", &secs) != 0);
  CHECK(ClockCalc_ParseDuration("h", &secs) != 0);
  CHECK(ClockCalc_ParseDuration("10x", &secs) != 0);
  CHECK(ClockCalc_ParseDuration("0m", &secs) != 0);
  CHECK(ClockCalc_ParseDuration("99999999d", &secs) != 0);
}

static void test_crc16(void)
{
  CHECK(Crc16("123456789", 9) == 0x29B1u);
  CHECK(Crc16("", 0) == 0xFFFFu);
}

int main(void)
{
  test_clock();
  test_tokenizer();
  test_rtc_trim();
  test_time_entry();
  test_seconds_and_durations();
  test_crc16();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}

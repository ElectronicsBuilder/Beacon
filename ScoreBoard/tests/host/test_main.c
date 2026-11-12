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
#include "msg_queue.h"
#include "text_entry.h"
#include "end_plan.h"

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

  char big[12];
  CHECK(ClockCalc_FormatBig(3600u + 120u + 3u, big, sizeof big) == 0 && strcmp(big, "01:02:03") == 0);
  CHECK(ClockCalc_FormatBig(49u * 86400u + 5u * 3600u + 12u * 60u + 33u, big, sizeof big) == 49 && strcmp(big, "49D 05:12") == 0);
  CHECK(ClockCalc_FormatBig(123u * 86400u + 5u * 3600u + 9u * 60u, big, sizeof big) == 123 && strcmp(big, "123D 05:09") == 0);
  CHECK(ClockCalc_FormatBig(1234u * 86400u + 7u * 3600u, big, sizeof big) == 1234 && strcmp(big, "1234D 07H") == 0);
  CHECK(strlen(big) <= 10u);

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

static MsgQueue q;

static void test_msg_queue(void)
{
  MsgQueue_Init(&q, 4);
  CHECK(MsgQueue_Count(&q) == 0);
  CHECK(MsgQueue_Next(&q, MSG_ORDER_SEQ, 0) == -1);

  CHECK(MsgQueue_Add(&q, "HELLO", NULL, 1) == 0);
  CHECK(MsgQueue_Add(&q, "LINE ONE", "LINE TWO", 2) == 1);
  CHECK(MsgQueue_Add(&q, "THIRD", "", 0) == 2);
  CHECK(MsgQueue_Count(&q) == 3);
  CHECK(strcmp(q.slots[1].line2, "LINE TWO") == 0 && q.slots[1].color == 2);

  /* invalid text */
  CHECK(MsgQueue_Add(&q, "", NULL, 0) == -2);
  CHECK(MsgQueue_Add(&q, "TAB\tX", NULL, 0) == -2);
  CHECK(MsgQueue_Add(&q, "0123456789012345678901234567890123456789X", NULL, 0) == -2);   /* 41 chars */
  CHECK(MsgQueue_Add(&q, "0123456789012345678901234567890123456789", NULL, 0) == 3);     /* 40 ok */
  CHECK(MsgQueue_Add(&q, "FULL", NULL, 0) == -1);

  /* sequential order wraps */
  CHECK(MsgQueue_Next(&q, MSG_ORDER_SEQ, 0) == 0);
  CHECK(MsgQueue_Next(&q, MSG_ORDER_SEQ, 0) == 1);
  CHECK(MsgQueue_Next(&q, MSG_ORDER_SEQ, 0) == 2);
  CHECK(MsgQueue_Next(&q, MSG_ORDER_SEQ, 0) == 3);
  CHECK(MsgQueue_Next(&q, MSG_ORDER_SEQ, 0) == 0);
  CHECK(MsgQueue_Prev(&q) == 3);

  /* delete frees the slot; a new message goes there but plays last */
  CHECK(MsgQueue_Delete(&q, 1) == 0);
  CHECK(MsgQueue_Delete(&q, 1) != 0);
  CHECK(MsgQueue_Add(&q, "NEW", NULL, 0) == 1);
  uint8_t order[MSG_MAX];
  CHECK(MsgQueue_Sorted(&q, order) == 4);
  CHECK(order[0] == 0 && order[1] == 2 && order[2] == 3 && order[3] == 1);

  /* random never repeats the current one */
  int repeats = 0, seen[4] = {0};
  q.current = 0;
  for (uint32_t r = 0; r < 200; r++)
  {
    int before = q.current;
    int n = MsgQueue_Next(&q, MSG_ORDER_RANDOM, r * 2654435761u);
    if (n == before) repeats++;
    if (n >= 0 && n < 4) seen[n]++;
  }
  CHECK(repeats == 0 && seen[0] && seen[1] && seen[2] && seen[3]);

  /* one message: random returns it */
  MsgQueue_Init(&q, 4);
  MsgQueue_Add(&q, "ONLY", NULL, 0);
  CHECK(MsgQueue_Next(&q, MSG_ORDER_RANDOM, 7) == 0);
  CHECK(MsgQueue_Next(&q, MSG_ORDER_RANDOM, 9) == 0);

  /* loaded from storage: seq continues after the highest */
  MsgQueue_Init(&q, 3);
  q.slots[2].seq = 41; strcpy(q.slots[2].line1, "A");
  q.slots[0].seq = 7;  strcpy(q.slots[0].line1, "B");
  MsgQueue_Loaded(&q);
  CHECK(q.next_seq == 42);
  CHECK(MsgQueue_Next(&q, MSG_ORDER_SEQ, 0) == 0);    /* seq 7 first */
  q.next_seq = 0xFF00u;
  CHECK(MsgQueue_NeedsRenumber(&q));
  MsgQueue_Renumber(&q);
  CHECK(q.slots[0].seq == 1 && q.slots[2].seq == 2 && q.next_seq == 3);
}

static void test_text_entry(void)
{
  TextEntry e;
  TextEntry_Start(&e, "HI", 5);
  CHECK(e.len == 2);
  TextEntry_Key(&e, ' '); TextEntry_Key(&e, 'Y'); TextEntry_Key(&e, 'O'); TextEntry_Key(&e, 'U');
  CHECK(strcmp(e.text, "HI YO") == 0);          /* stops at max */
  TextEntry_Key(&e, KB_BACK);
  CHECK(strcmp(e.text, "HI Y") == 0);
  CHECK(TextEntry_Key(&e, 0x81) == TE_EDITING && strcmp(e.text, "HI Y") == 0);   /* F1 ignored */
  CHECK(TextEntry_Key(&e, KB_ENTER) == TE_DONE);
  CHECK(TextEntry_Key(&e, KB_ESC) == TE_CANCEL);
  for (int i = 0; i < 10; i++) TextEntry_Key(&e, KB_BACK);
  CHECK(e.len == 0 && e.text[0] == '\0');
}

static void test_playlist_and_end_plan(void)
{
  MsgQueue_Init(&q, 6);
  for (int i = 0; i < 6; i++) MsgQueue_Add(&q, "M", NULL, 0);
  uint32_t mask = (1u << 1) | (1u << 4);         /* ids 2 and 5 */
  CHECK(MsgQueue_NextIn(&q, MSG_ORDER_SEQ, 0, mask) == 1);
  CHECK(MsgQueue_NextIn(&q, MSG_ORDER_SEQ, 0, mask) == 4);
  CHECK(MsgQueue_NextIn(&q, MSG_ORDER_SEQ, 0, mask) == 1);
  q.current = 0;                                 /* current outside the list: first of the list */
  CHECK(MsgQueue_NextIn(&q, MSG_ORDER_SEQ, 0, mask) == 1);
  for (uint32_t r = 0; r < 50; r++)
  {
    int n = MsgQueue_NextIn(&q, MSG_ORDER_RANDOM, r * 7919u, mask);
    CHECK(n == 1 || n == 4);
  }
  CHECK(MsgQueue_NextIn(&q, MSG_ORDER_SEQ, 0, 1u << 20) == -1);   /* no such message */

  EndPlan p = END_PLAN_DEFAULT;
  CHECK(EndPlan_Phase(&p, 0) == END_PHASE_THEN);                  /* no scene */
  p.scene = SCENE_FIREWORKS; p.scene_s = 30;
  CHECK(EndPlan_Phase(&p, 0) == END_PHASE_SCENE);
  CHECK(EndPlan_Phase(&p, 29) == END_PHASE_SCENE);
  CHECK(EndPlan_Phase(&p, 30) == END_PHASE_THEN);
  p.scene_s = 0;                                                  /* scene without end */
  CHECK(EndPlan_Phase(&p, 1000000u) == END_PHASE_SCENE);
  CHECK(EndPlan_SceneFind("snow") == SCENE_SNOW && EndPlan_SceneFind("x") < 0);
  CHECK(EndPlan_ThenFind("screen") == END_THEN_SCREEN);
  CHECK(strcmp(EndPlan_SceneName(SCENE_RAINBOW), "rainbow") == 0);
  p.then = 7;
  CHECK(!EndPlan_Valid(&p));
}

int main(void)
{
  test_clock();
  test_tokenizer();
  test_rtc_trim();
  test_time_entry();
  test_seconds_and_durations();
  test_crc16();
  test_msg_queue();
  test_text_entry();
  test_playlist_and_end_plan();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}

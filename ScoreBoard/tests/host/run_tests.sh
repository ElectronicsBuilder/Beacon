#!/bin/sh
# Build and run the host unit tests (hardware-independent firmware logic).
# Uses $CC if set, otherwise gcc (e.g. C:/cygwin64/bin/gcc.exe on this PC).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
APP="$HERE/../../App"
CC=${CC:-gcc}
OUT="$HERE/build"
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Werror -O1 \
  -I"$APP/Services" -I"$APP/Console" \
  "$HERE/test_main.c" "$APP/Services/clock_calc.c" "$APP/Console/cli_parse.c" \
  "$APP/Services/rtc_trim_calc.c" "$APP/Services/time_entry.c" \
  -o "$OUT/host_tests"
"$OUT/host_tests"

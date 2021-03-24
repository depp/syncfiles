set -e
CFLAGS="-O0 -g -Wall -Wextra -Wstrict-prototypes"
cc -o convert_test $CFLAGS convert_test.c mac_to_unix.c mac_from_unix.c
exec ./convert_test

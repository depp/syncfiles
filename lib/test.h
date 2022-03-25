#ifndef test_h
#define test_h
/* test.h - unit testing definitions. */

#include "lib/defs.h"

/* Print an error message and exit. */
void Dief(const char *msg, ...) __attribute__((noreturn, format(printf, 1, 2)));

/* Print an error message with an error code and exit. */
void DieErrorf(int errcode, const char *msg, ...)
	__attribute__((noreturn, format(printf, 2, 3)));

#endif

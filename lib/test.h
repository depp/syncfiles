#ifndef test_h
#define test_h
/* test.h - unit testing definitions. */

#include "lib/defs.h"

/* The number of test failures. */
extern int gFailCount;

/* Set the name of the current test. */
void SetTestName(const char *name);

/* Set the name of the current test. */
void SetTestNamef(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Fail the current test. */
void Failf(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/* Return the description of an error code. Fatal error if the error code is
   invalid. */
const char *ErrorDescriptionOrDie(ErrorCode err);

/* Print information about completed tests and return the status code. */
int TestsDone(void );

#endif

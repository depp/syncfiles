#ifndef lib_util_h
#define lib_util_h
#include "lib/defs.h"

/* Print an error message and exit. */
void Fatalf(const char *msg, ...)
	__attribute__((noreturn, format(printf, 1, 2)));

/* Return a basic description of the given error code, or NULL if the error code
   is unknown. */
const char *ErrorDescription(ErrorCode err);

#endif

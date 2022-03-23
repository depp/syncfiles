/* toolbox.c - replacement functions for Mac OS toolbox functions

   This is used to run conversion tests on non-Mac OS systems to make
   development easier. These are not intended to make it possible to port the
   converter to non-Mac OS systems. */
#include "convert/defs.h"
#include "convert/test.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Dief(const char *msg, ...)
{
	va_list ap;
	fputs("Error: ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

void DieErrorf(int errcode, const char *msg, ...)
{
	va_list ap;
	fputs("Error: ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputs(": ", stderr);
	fputs(strerror(errcode), stderr);
	fputc('\n', stderr);
	exit(1);
}

Handle NewHandle(Size byteCount)
{
	Ptr p;
	Handle h;

	if (byteCount < 0) {
		Dief("NewHandle: byteCount = %ld", byteCount);
	}
	p = malloc(byteCount);
	if (byteCount > 0 && p == NULL) {
		Dief("NewHandle: malloc failed");
	}
	h = malloc(sizeof(Ptr));
	if (h == NULL) {
		Dief("NewHandle: malloc failed");
	}
	*h = p;
	return h;
}

void HLock(Handle h)
{
	(void)h;
}

void HUnlock(Handle h)
{
	(void)h;
}

void DisposeHandle(Handle h)
{
	if (h != NULL) {
		free(*h);
		free(h);
	}
}

void SetHandleSize(Handle h, Size newSize)
{
	Ptr p;
	if (h == NULL) {
		Dief("SetHandleSize: h = NULL");
	}
	p = realloc(*h, newSize);
	if (newSize > 0 && p == NULL) {
		Dief("SetHandleSize: realloc failed");
	}
	*h = p;
}

OSErr MemError(void)
{
	/* Memory allocation failures abort the program. */
	return 0;
}

void MemClear(void *ptr, Size size)
{
	memset(ptr, 0, size);
}

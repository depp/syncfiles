// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
/* Defined to get vsnprintf. */
#define _XOPEN_SOURCE 500

#include "lib/test.h"
#include "lib/util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int gFailCount;

static char gTestName[256];

void SetTestName(const char *name)
{
	size_t n;

	n = strlen(name);
	if (n >= sizeof(gTestName) - 1) {
		Fatalf("SetTestName: overflow");
	}
	memcpy(gTestName, name, n + 1);
}

void SetTestNamef(const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = vsnprintf(gTestName, sizeof(gTestName), fmt, ap);
	va_end(ap);

	if (r >= (int)sizeof(gTestName)) {
		Fatalf("SetTestNamef: overflow");
	}
}

void Failf(const char *msg, ...)
{
	va_list ap;

	gFailCount++;
	fputs("Error: ", stderr);
	fputs(gTestName, stderr);
	fputs(": ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);
	if (gFailCount >= 10) {
		exit(1);
	}
}

const char *ErrorDescriptionOrDie(ErrorCode err)
{
	const char *desc;

	desc = ErrorDescription(err);
	if (desc == NULL) {
		Fatalf("invalid error code: %d", err);
	}
	return desc;
}

int TestsDone(void)
{
	if (gFailCount > 0) {
		fputs("failed\n", stderr);
		return 1;
	}
	fputs("ok\n", stderr);
	return 0;
}

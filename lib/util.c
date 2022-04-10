// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "lib/util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void Fatalf(const char *msg, ...)
{
	va_list ap;
	fputs("Error: ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

static const char *const kErrorNames[] = {
	"ok",
	"no memory",
	"bad data",
	"too many files in one directory",
};

const char *ErrorDescription(ErrorCode err)
{
	if (err < 0 || (int)(sizeof(kErrorNames) / sizeof(*kErrorNames)) <= err) {
		return NULL;
	}
	return kErrorNames[err];
}

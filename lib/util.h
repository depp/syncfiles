// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef LIB_UTIL_H
#define LIB_UTIL_H
#include "lib/error.h"

// Print an error message and exit.
void Fatalf(const char *msg, ...)
	__attribute__((noreturn, format(printf, 1, 2)));

// Return a basic description of the given error code, or NULL if the error code
// is unknown.
const char *ErrorDescription(ErrorCode err);

#endif

// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef LIB_TEST_H
#define LIB_TEST_H
// test.h - unit testing definitions.

#include "lib/defs.h"
#include "lib/error.h"

// The number of test failures.
extern int gFailCount;

// Set the name of the current test.
void SetTestName(const char *name);

// Set the name of the current test.
void SetTestNamef(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Fail the current test.
void Failf(const char *msg, ...) __attribute__((format(printf, 1, 2)));

// Return the description of an error code. Fatal error if the error code is
// invalid.
const char *ErrorDescriptionOrDie(ErrorCode err);

// Print information about completed tests and return the status code.
int TestsDone(void);

#endif

// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_ERROR_H
#define MACOS_ERROR_H

// Error codes, corresponding to messages in a STR# resource. This should be
// kept in sync with STR# rSTRS_Errors in resources.r.
typedef enum {
	// An unknown error occurred.
	kErrUnknown = 1,
	// An internal error occurred.
	kErrInternal,
	// Out of memory.
	kErrOutOfMemory,
	// Could not save project "^1".
	kErrCouldNotSaveProject,
} ErrorCode;

// ExitError shows an error dialog with the given error message, then quits the
// program.
void ExitError(ErrorCode errCode);

// ExitErrorOS shows an error dialog with the given error message and an OS
// error code, then quits the program.
void ExitErrorOS(ErrorCode errCode, short osErr);

// ExitMemError shows an out of memory error and quits the program.
void ExitMemError(void);

// ExitAssert shows an assertion error and quits the program. The message may be
// NULL.
void ExitAssert(const unsigned char *file, int line,
                const unsigned char *message);

// EXIT_ASSERT shows an assertion error and quits the program. The message may
// be NULL.
#define EXIT_ASSERT(str) ExitAssert("\p" __FILE__, __LINE__, str)

// ASSERT checks that the given condition is true. Otherwise, it displays an
// error message and quits the program.
#define ASSERT(p)                                         \
	do {                                                  \
		if (!(p))                                         \
			ExitAssert("\p" __FILE__, __LINE__, "\p" #p); \
	} while (0)

// An ErrorParams contains the parameters for displaying en error alert window.
// This structure should be zeroed before use, in case additional fields are
// added.
struct ErrorParams {
	// The application error code. If this is zero, it will be treated as
	// kErrInternal.
	ErrorCode err;

	// The OS error code. This is displayed at the end of the error message,
	// unless it is zero.
	short osErr;

	// If the error messages contain any string substitutions like "^1", those
	// substitutions are replaced with this string.
	const unsigned char *strParam;
};

// ShowError shows an error alert window to the user.
void ShowError(const struct ErrorParams *p);

#endif

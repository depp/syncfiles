// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_ERROR_H
#define MACOS_ERROR_H

// Error codes, corresponding to messages in a STR# resource. This should be
// kept in sync with STR# rSTRS_Errors in resources.r.
typedef enum ErrorCode {
	// (no error)
	kErrNone,
	// Out of memory.
	kErrOutOfMemory,
	// Could not save project "^1".
	kErrCouldNotSaveProject,
	// Could not read project "^1".
	kErrCouldNotReadProject,
	// The project file is damaged.
	kErrProjectDamaged,
	// The project data is too large.
	kErrProjectLarge,
	// The project file is from an unknown version of SyncFiles.
	kErrProjectUnknownVersion,
	// Could not query volume parameters.
	kErrVolumeQuery,
	// Could not create alias.
	kErrAlias,
	// Could not get directory path.
	kErrDirPath
} ErrorCode;

// ExitAssert shows an assertion error and quits the program. Either the file or
// the assertion may be NULL.
void ExitAssert(const unsigned char *file, int line,
                const unsigned char *assertion);

// EXIT_INTERNAL shows an internal error message and quits the program.
#define EXIT_INTERNAL() ExitAssert("\p" __FILE__, __LINE__, NULL)

// ASSERT checks that the given condition is true. Otherwise, it displays an
// error message and quits the program.
#define ASSERT(p)                                         \
	do {                                                  \
		if (!(p))                                         \
			ExitAssert("\p" __FILE__, __LINE__, "\p" #p); \
	} while (0)

// ShowMemError shows an out of memory error to the user.
void ShowMemError(void);

// ShowError shows an error alert window to the user. The error codes are
// displayed if they are not 0. If osErr is not 0, then it is displayed as well.
// Any ^1 parameters in the error messages are replaced with strParam.
void ShowError(ErrorCode err1, ErrorCode err2, short osErr,
               const unsigned char *strParam);

#endif

// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef LIB_ERROR_H
#define LIB_ERROR_H

// Error codes.
typedef enum {
	// No error (success). Equal to 0.
	kErrorOK,

	// Memory allocation failed.
	kErrorNoMemory,

	// Invaild table data.
	kErrorBadData,

	// Too many files in one directory.
	kErrorDirectoryTooLarge,
} ErrorCode;

#endif

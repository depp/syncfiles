// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef SYNC_META_H
#define SYNC_META_H
/* meta.h - file metadata */

#include "lib/defs.h"

#if TARGET_API_MAC_OS8
struct Timespec {
	UInt32 mactime;
};
#else
struct Timestamp {
	int64_t sec;
	int32_t nsec;
};
#endif

typedef enum {
	kTypeNotExist,
	kTypeFile,
	kTypeDirectory,
} FileType;

enum {
	kLocal,
	kRemote,
};

enum {
	/* Maximum length of a filename. This is a Mac OS limitation. */
	kFilenameLength = 31,

	/* Size of a filename record in 32-bit units. */
	kFilenameSizeU32 = (kFilenameLength + 3) / 4
};

/* The name of a file, as a Pascal string -- the first byte is the string
   length, 0-31. The name must be padded with zeroes. */
typedef union {
	UInt8 u8[kFilenameSizeU32 * 4];
	UInt32 u32[kFilenameSizeU32];
} FileName;

/* Metadata for a file or directory. */
struct Metadata {
	FileType type;
	UInt32 size;
	struct Timestamp mod_time;
};

/* Information about a local and remote file or directory. */
struct FileRec {
	FileName name[2];

	/* Metadata for kLocal and kRemote. */
	struct Metadata meta[2];

	/* Type and creator code for local copy. */
	UInt32 type_code;
	UInt32 creator_code;
};

#endif

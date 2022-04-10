// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "tempfile.h"

#include <string.h>

// Filename prefix for temporary files.
static const char kTempPrefix[] = "SyncFiles.";

// Hexadecimal digits.
static const char kHexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static void U32ToHex(unsigned char *ptr, UInt32 x)
{
	int i;
	for (i = 0; i < 8; i++) {
		ptr[7 - i] = kHexDigits[x & 15];
		x >>= 4;
	}
}

OSErr MakeTempFile(short vRefNum, FSSpec *spec)
{
	short tempVRefNum;
	long tempParID;
	unsigned long time;
	OSErr err;
	int pos;
	unsigned char name[32];

	// This is inspired by IM: Files listing 1-10, page 1-25.

	// Generate hopefully unique name.
	GetDateTime(&time);
	memcpy(name + 1, kTempPrefix, sizeof(kTempPrefix) - 1);
	pos = sizeof(kTempPrefix);
	U32ToHex(name + pos, time);
	pos += 8;
	name[0] = pos - 1;

	err = FindFolder(vRefNum, kTemporaryFolderType, TRUE, &tempVRefNum,
	                 &tempParID);
	if (err != noErr) {
		return err;
	}
	err = FSMakeFSSpec(tempVRefNum, tempParID, name, spec);
	if (err != noErr && err != fnfErr) {
		return err;
	}
	return noErr;
}

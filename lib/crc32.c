// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "lib/crc32.h"

static int gCRCInitted;
static UInt32 gCRCTable[256];

static void CRC32Init(void)
{
	int i, j;
	UInt32 v, u;

	for (i = 0; i < 256; i++) {
		v = i;
		for (j = 0; j < 8; j++) {
			u = v;
			v >>= 1;
			if (u & 1) {
				v ^= 0xedb88320;
			}
		}
		gCRCTable[i] = v;
	}
	gCRCInitted = 1;
}

UInt32 CRC32Update(UInt32 crc, const void *ptr, Size size)
{
	const UInt8 *p, *e;

	if (!gCRCInitted) {
		CRC32Init();
	}

	crc = ~crc;
	p = ptr;
	e = p + size;
	while (p < e) {
		crc = (crc >> 8) ^ gCRCTable[(crc & 0xff) ^ *p++];
	}
	return ~crc;
}

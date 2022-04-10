// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "lib/endian.h"

#include "lib/test.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static void EndianFail(const char *name, UInt32 got, UInt32 expect)
{
	fprintf(stderr, "%s = 0x%" PRIx32 ", expect 0x%" PRIx32 "\n", name, got,
	        expect);
	gFailCount++;
}

#define CHECK(fn, expect)            \
	be = fn(u.i);                    \
	if (be != expect) {              \
		EndianFail(#fn, be, expect); \
	}

static void Test16(void)
{
	union {
		UInt16 i;
		UInt8 b[2];
	} u;
	UInt16 be;

	u.b[0] = 0x12;
	u.b[1] = 0x34;

	CHECK(EndianU16_BtoN, 0x1234)
	CHECK(EndianU16_NtoB, 0x1234)
}

static void Test32(void)
{
	union {
		UInt32 i;
		UInt8 b[2];
	} u;
	UInt32 be;

	u.b[0] = 0x12;
	u.b[1] = 0x34;
	u.b[2] = 0x56;
	u.b[3] = 0x78;

	CHECK(EndianU32_BtoN, 0x12345678)
	CHECK(EndianU32_NtoB, 0x12345678)
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	Test16();
	Test32();
	return TestsDone();
}

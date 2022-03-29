#include "lib/endian.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int gDidFail;

static void Fail(const char *name, UInt32 got, UInt32 expect)
{
	fprintf(stderr, "%s = 0x%" PRIx32 ", expect 0x%" PRIx32 "\n", name, got,
	        expect);
	gDidFail = 1;
}

#define CHECK(fn, expect)      \
	be = fn(u.i);              \
	if (be != expect) {        \
		Fail(#fn, be, expect); \
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
	return gDidFail;
}

// Copyright 2023 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "macos/pstrbuilder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// Temporary buffer for Pascal strings

static size_t PStrBufPos;
static unsigned char PStrBuf[1024];

static const unsigned char *P2CStr(const char *str)
{
	size_t len;
	unsigned char *ptr;

	len = strlen(str);
	if (len > 255) {
		fputs("Error: PStr too long\n", stderr);
		exit(1);
	}
	if (len + 1 > sizeof(PStrBuf) - PStrBufPos) {
		fputs("Error: PStr overflow\n", stderr);
		exit(1);
	}
	ptr = PStrBuf + PStrBufPos;
	*ptr = len;
	memcpy(ptr + 1, str, len);
	PStrBufPos += len + 1;
	return ptr;
}

static void PStrReset(void)
{
	PStrBufPos = 0;
}

#define PSTR(x) P2CStr(x)
#define PSTR_RESET() PStrReset()

// =============================================================================

static Boolean TestFailed;

static void PrintQuoted(const unsigned char *str)
{
	const unsigned char *ptr = str + 1;
	const unsigned char *end = ptr + str[0];
	int ch;

	for (; ptr < end; ptr++) {
		ch = *ptr;
		if (32 <= ch && ch <= 126) {
			if (ch == '\\' || ch == '"') {
				fputc('\\', stderr);
			}
			fputc(ch, stderr);
		} else {
			fprintf(stderr, "\\x%02x", ch);
		}
	}
}

static void TestString(int lineno, const struct PStrBuilder *buf,
                       const unsigned char *expect, Boolean expect_truncated)
{
	if (memcmp(buf->data, expect, expect[0] + 1) != 0 ||
	    buf->truncated != expect_truncated) {
		fprintf(stderr, "%s:%d: Incorrect result\n", __FILE__, lineno);
		fputs("  expect: \"", stderr);
		PrintQuoted(expect);
		fprintf(stderr, "\" (truncated=%d)\n", expect_truncated);
		fputs("     got: \"", stderr);
		PrintQuoted(buf->data);
		fprintf(stderr, "\" (truncated=%d)\n", buf->truncated);
		TestFailed = true;
	}
}

#define TEST_STRING(buf, expect, expect_truncated) \
	TestString(__LINE__, buf, expect, expect_truncated)

#define STR50 "0123456789abcdef0123456789abcdef0123456789abcdef0."
#define STR250 STR50 STR50 STR50 STR50 STR50

static void Clear(struct PStrBuilder *buf, const unsigned char **params)
{
	int i;
	buf->data[0] = 0;
	buf->truncated = false;
	for (i = 0; i < 9; i++) {
		params[i] = NULL;
	}
}

int main(int argc, char **argv)
{
	struct PStrBuilder buf;
	const unsigned char *params[9];

	(void)argc;
	(void)argv;

	Clear(&buf, params);
	TEST_STRING(&buf, PSTR(""), false);
	PSTR_RESET();

	// Append string to empty buffer.
	PStrAppend(&buf, PSTR("String1"));
	TEST_STRING(&buf, PSTR("String1"), false);
	PSTR_RESET();

	// Append string to non-empty buffer.
	PStrAppend(&buf, PSTR("String2"));
	TEST_STRING(&buf, PSTR("String1String2"), false);
	PSTR_RESET();

	// Append string with parameters.
	Clear(&buf, params);
	PStrAppend(&buf, PSTR("initial: "));
	params[0] = PSTR("(param1)");
	params[1] = PSTR("(param2)");
	params[8] = PSTR("(param9)");
	PStrAppendSubstitute(&buf, PSTR("^1 ^9 -> ^2"), 9, params);
	TEST_STRING(&buf, PSTR("initial: (param1) (param9) -> (param2)"), false);
	PSTR_RESET();

	// Missing parameters are left alone, including NULL parameters.
	Clear(&buf, params);
	params[0] = PSTR("param1");
	PStrAppendSubstitute(&buf, PSTR("^0 ^1 ^2 ^3 ^9"), 2, params);
	TEST_STRING(&buf, PSTR("^0 param1 ^2 ^3 ^9"), false);

	// String length 255 works.
	Clear(&buf, params);
	PStrAppend(&buf, PSTR(STR250));
	PStrAppend(&buf, PSTR("VWXYZ"));
	TEST_STRING(&buf, PSTR(STR250 "VWXYZ"), false);
	PSTR_RESET();

	// String length 256 is truncated.
	PStrAppend(&buf, PSTR("."));
	TEST_STRING(&buf, PSTR(STR250 "VWXY\xc9"), true);
	PSTR_RESET();

	// Truncate with parameters.
	Clear(&buf, params);
	params[0] = PSTR(STR50);
	PStrAppendSubstitute(&buf, PSTR("^1^1^1^1^1^1"), 1, params);
	TEST_STRING(&buf, PSTR(STR250 "0123\xc9"), true);
	PSTR_RESET();

	fputs("OK?\n", stderr);
	if (TestFailed) {
		fputs("Test failed\n", stderr);
		exit(1);
	}
	return 0;
}

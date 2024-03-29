// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "convert/convert.h"
#include "convert/data.h"
#include "lib/test.h"
#include "lib/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	kInitialBufSize = 4 * 1024,
	kConvertBufferSize = 1024,
};

static UInt8 *gBuffer[3];

static void PrintQuotedString(const UInt8 *buf, int len)
{
	int i, c;

	fputc('"', stderr);
	for (i = 0; i < len; i++) {
		c = buf[i];
		if (32 <= c && c <= 126) {
			if (c == '\\' || c == '"') {
				fputc('\\', stderr);
			}
			fputc(c, stderr);
		} else {
			fprintf(stderr, "\\x%02x", c);
		}
	}
	fputc('"', stderr);
}

static void Check(const void *exbuf, int exlen, const void *inbuf, int inlen,
                  const void *outbuf, int outlen)
{
	int i, n, col, diffcol, c1, c2;

	if (exlen == outlen && memcmp(exbuf, outbuf, outlen) == 0) {
		return;
	}
	Failf("incorrect output");
	n = exlen;
	if (n > outlen) {
		n = outlen;
	}
	diffcol = -1;
	col = 0;
	for (i = 0; i < n; i++) {
		c1 = ((const UInt8 *)exbuf)[i];
		c2 = ((const UInt8 *)outbuf)[i];
		if (c1 != c2) {
			diffcol = col;
			break;
		}
		if (32 <= c1 && c1 <= 126) {
			col++;
			if (c1 == '\\' || c1 == '"') {
				col++;
			}
		} else {
			col += 4;
		}
	}
	fputs("Input:  ", stderr);
	PrintQuotedString(inbuf, inlen);
	fputc('\n', stderr);
	fputs("Expect: ", stderr);
	PrintQuotedString(exbuf, exlen);
	fputc('\n', stderr);
	fputs("Output: ", stderr);
	PrintQuotedString(outbuf, outlen);
	fputc('\n', stderr);
	if (diffcol >= 0) {
		for (i = 0; i < diffcol + 9; i++) {
			fputc(' ', stderr);
		}
		fputc('^', stderr);
	}
	fputc('\n', stderr);
}

static const char *const kLineBreakData[4] = {
	"Line Break\nA\n\nB\rC\r\rD\r\nE\r\n\r\n",
	"Line Break\nA\n\nB\nC\n\nD\nE\n\n",
	"Line Break\rA\r\rB\rC\r\rD\rE\r\r",
	"Line Break\r\nA\r\n\r\nB\r\nC\r\n\r\nD\r\nE\r\n\r\n",
};

static const char *const kLineBreakName[4] = {"keep", "LF", "CR", "CRLF"};

static void TestConverter(const char *name, struct CharmapData data)
{
	Ptr datap;
	Handle datah;
	struct Converter cf, cr, cc;
	struct ConverterState st;
	int i, j, k, jmax, len0, len1, len2;
	UInt8 *ptr;
	const UInt8 *iptr, *iend, *istart;
	UInt8 *optr, *oend;
	int lblen[4];
	ErrorCode err;

	cf.data = NULL;
	cr.data = NULL;

	SetTestName(name);

	// Load the converter into memory and build the conversion table.
	datap = (void *)data.ptr;
	datah = &datap;
	err = ConverterBuild(&cf, datah, data.size, kToUTF8);
	if (err != 0) {
		Failf("ConverterBuild: to UTF-8: %s", ErrorDescriptionOrDie(err));
		goto done;
	}
	err = ConverterBuild(&cr, datah, data.size, kFromUTF8);
	if (err != 0) {
		Failf("ConverterBuild: from UTF-8: %s", ErrorDescriptionOrDie(err));
		goto done;
	}

	// Create sample data to convert: 0-255, followed by 0.
	len0 = 257;
	ptr = gBuffer[0];
	for (i = 0; i < 256; i++) {
		ptr[i] = i;
	}
	ptr[256] = 0;

	// Convert sample data.
	iptr = gBuffer[0];
	iend = iptr + 257;
	optr = gBuffer[1];
	oend = optr + kConvertBufferSize;
	st.data = 0;
	cf.run(*cf.data, kLineBreakKeep, &st, &optr, oend, &iptr, iend);
	if (iptr != iend) {
		Failf("some data failed to convert");
		goto done;
	}
	len1 = optr - gBuffer[1];

	// Convert back, in three calls. The middle call will be to a 1-4 byte slice
	// in the middle.
	for (i = 1; i < len1 - 2; i++) {
		jmax = len1 - i;
		if (jmax > 4) {
			jmax = 4;
		}
		for (j = 1; j <= jmax; j++) {
			SetTestNamef("%s reverse i=%d j=%d", name, i, j);
			st.data = 0;
			iptr = gBuffer[1];
			optr = gBuffer[2];
			oend = optr + kConvertBufferSize;
			iend = gBuffer[1] + i;
			cr.run(*cr.data, kLineBreakKeep, &st, &optr, oend, &iptr, iend);
			iend = gBuffer[1] + i + j;
			cr.run(*cr.data, kLineBreakKeep, &st, &optr, oend, &iptr, iend);
			iend = gBuffer[1] + len1;
			cr.run(*cr.data, kLineBreakKeep, &st, &optr, oend, &iptr, iend);
			if (iptr != iend) {
				Failf("some data failed to convert");
			} else {
				len2 = optr - gBuffer[2];
				Check(gBuffer[0], len0, gBuffer[1], len1, gBuffer[2], len2);
			}
		}
	}

	for (i = 0; i < 4; i++) {
		lblen[i] = strlen(kLineBreakData[i]) + 1;
	}
	istart = (const UInt8 *)kLineBreakData[0];
	for (k = 0; k < 2; k++) {
		cc = k == 0 ? cf : cr;
		for (i = 0; i < 4; i++) {
			len1 = lblen[0]; // Input data
			len0 = lblen[i]; // Expected output
			for (j = 1; j < len1; j++) {
				SetTestNamef("%s %s linebreak %s split=%d", name,
				             k == 0 ? "forward" : "backward", kLineBreakName[i],
				             j);
				st.data = 0;
				iptr = istart;
				optr = gBuffer[0];
				oend = optr + kConvertBufferSize;
				iend = istart + j;
				cc.run(*cc.data, i, &st, &optr, oend, &iptr, iend);
				iend = istart + len1;
				cc.run(*cc.data, i, &st, &optr, oend, &iptr, iend);
				if (iptr != iend) {
					Failf("some data failed to convert");
				} else {
					len2 = optr - gBuffer[0];
					Check(kLineBreakData[i], len0, kLineBreakData[0], len1,
					      gBuffer[0], len2);
				}
			}
		}
	}

done:
	if (cf.data != NULL) {
		DisposeHandle(cf.data);
	}
	if (cr.data != NULL) {
		DisposeHandle(cr.data);
	}
}

int main(int argc, char **argv)
{
	void *buf;
	struct CharmapData data;
	const char *name;
	int i;

	(void)argc;
	(void)argv;

	for (i = 0; i < 3; i++) {
		buf = malloc(kConvertBufferSize);
		if (buf == NULL) {
			Fatalf("malloc failed");
		}
		gBuffer[i] = buf;
	}

	for (i = 0;; i++) {
		name = CharmapName(i);
		if (name == NULL) {
			break;
		}
		data = CharmapData(i);
		if (data.ptr != NULL) {
			TestConverter(name, data);
		}
	}

	for (i = 0; i < 3; i++) {
		free(gBuffer[i]);
	}

	return TestsDone();
}

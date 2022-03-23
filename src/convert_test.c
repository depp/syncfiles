/* Converter test. */
#define _XOPEN_SOURCE 500

#include "src/convert.h"
#include "src/test.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	kInitialBufSize = 4 * 1024,
	kConvertBufferSize = 1024
};

static int gFailCount;
static char gTestName[128];

static void Failf(const char *msg, ...) __attribute__((format(printf, 1, 2)));

static void Failf(const char *msg, ...)
{
	va_list ap;

	gFailCount++;
	fputs("Error: ", stderr);
	fputs(gTestName, stderr);
	fputs(": ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);
	if (gFailCount >= 10) {
		exit(1);
	}
}

static const char *const kErrorNames[] = {"ok", "no memory", "bad data"};

static const char *ErrorName(int err)
{
	if (err < 0 || (int)(sizeof(kErrorNames) / sizeof(*kErrorNames)) <= err) {
		Dief("bad error code: %d", err);
	}
	return kErrorNames[err];
}

static void StringPrintf(char *dest, size_t destsz, const char *fmt, ...)
	__attribute__((format(printf, 3, 4)));

static void StringPrintf(char *dest, size_t destsz, const char *fmt, ...)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(dest, destsz, fmt, ap);
	va_end(ap);

	if (n < 0 || n >= (int)destsz) {
		Dief("snprintf: overflow");
	}
}

/* Read a file in its entirety. */
static void ReadFile(const char *filename, void **datap, size_t *sizep)
{
	char fnbuf[128];
	FILE *fp = NULL;
	char *buf = NULL, *newbuf;
	size_t size, alloc, newalloc, amt;
	int err;

	StringPrintf(fnbuf, sizeof(fnbuf), "src/%s", filename);

	fp = fopen(fnbuf, "rb");
	if (fp == NULL) {
		err = errno;
		goto error;
	}
	buf = malloc(kInitialBufSize);
	if (buf == NULL) {
		err = errno;
		goto error;
	}
	size = 0;
	alloc = kInitialBufSize;
	for (;;) {
		if (size >= alloc) {
			newalloc = alloc * 2;
			newbuf = realloc(buf, newalloc);
			if (newbuf == NULL) {
				err = errno;
				goto error;
			}
			alloc = newalloc;
			buf = newbuf;
		}
		amt = fread(buf + size, 1, alloc - size, fp);
		if (amt == 0) {
			if (feof(fp)) {
				break;
			}
			err = errno;
			goto error;
		}
		size += amt;
	}
	fclose(fp);
	*datap = buf;
	*sizep = size;
	return;

error:
	if (fp != NULL) {
		fclose(fp);
	}
	if (buf != NULL) {
		free(buf);
	}
	DieErrorf(err, "read %s", filename);
}

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

static void Check(int len0, int len1, int len2)
{
	int i, n, col, diffcol, c1, c2;

	if (len0 == len2 && memcmp(gBuffer[0], gBuffer[2], len2) == 0) {
		return;
	}
	Failf("incorrect output");
	n = len0;
	if (n > len2) {
		n = len2;
	}
	diffcol = -1;
	col = 0;
	for (i = 0; i < n; i++) {
		c1 = gBuffer[0][i];
		c2 = gBuffer[2][i];
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
	PrintQuotedString(gBuffer[1], len1);
	fputc('\n', stderr);
	fputs("Expect: ", stderr);
	PrintQuotedString(gBuffer[0], len0);
	fputc('\n', stderr);
	fputs("Output: ", stderr);
	PrintQuotedString(gBuffer[2], len2);
	fputc('\n', stderr);
	if (diffcol >= 0) {
		for (i = 0; i < diffcol + 9; i++) {
			fputc(' ', stderr);
		}
		fputc('^', stderr);
	}
	fputc('\n', stderr);
}

static void TestConverter(const char *filename)
{
	void *data;
	size_t datasz;
	Ptr datap;
	Handle datah;
	struct Converter cf, cr;
	struct ConverterState st;
	int r, i, j, jmax, len0, len1, len2;
	OSErr err;
	UInt8 *ptr;
	const UInt8 *iptr, *iend;
	UInt8 *optr, *oend;

	data = NULL;
	cf.data = NULL;
	cr.data = NULL;

	StringPrintf(gTestName, sizeof(gTestName), "%s", filename);

	/* Load the converter into memory and build the conversion table. */
	ReadFile(filename, &data, &datasz);
	datap = data;
	datah = &datap;
	r = ConverterBuild(&cf, datah, datasz, kToUTF8, &err);
	if (r != 0) {
		Failf("ConverterBuild: %s (to UTF-8): %s", filename, ErrorName(r));
		goto done;
	}
	r = ConverterBuild(&cr, datah, datasz, kFromUTF8, &err);
	if (r != 0) {
		Failf("ConverterBuild: %s (from UTF-8): %s", filename, ErrorName(r));
		goto done;
	}

	/* Create sample data to convert: 0-255, followed by 0. */
	len0 = 257;
	ptr = gBuffer[0];
	for (i = 0; i < 256; i++) {
		ptr[i] = i;
	}
	ptr[256] = 0;

	/* Convert sample data. */
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

	/* Convert back, in three calls. The middle call will be to a 1-4 byte slice
	   in the middle. */
	for (i = 1; i < len1 - 2; i++) {
		jmax = len1 - i;
		if (jmax > 4) {
			jmax = 4;
		}
		for (j = 1; j <= jmax; j++) {
			StringPrintf(gTestName, sizeof(gTestName), "%s reverse i=%d j=%d",
			             filename, i, j);
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
				continue;
			}
			len2 = optr - gBuffer[2];
			Check(len0, len1, len2);
		}
	}

done:
	free(data);
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
	const char *filename;
	int i;

	(void)argc;
	(void)argv;

	for (i = 0; i < 3; i++) {
		buf = malloc(kConvertBufferSize);
		if (buf == NULL) {
			DieErrorf(errno, "malloc");
		}
		gBuffer[i] = buf;
	}

	for (i = 0;; i++) {
		filename = kCharsetFilename[i];
		if (filename == NULL) {
			break;
		}
		TestConverter(filename);
	}

	for (i = 0; i < 3; i++) {
		free(gBuffer[i]);
	}

	return gFailCount == 0 ? 0 : 1;
}
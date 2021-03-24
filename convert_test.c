#include "convert.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#include "mac_from_unix_data.h"

static noreturn void malloc_fail(size_t sz) {
	fprintf(stderr, "Error: malloc(%zu) failed\n", sz);
	exit(1);
}

static void *xmalloc(size_t sz) {
	void *ptr = malloc(sz);
	if (ptr == NULL) {
		malloc_fail(sz);
	}
	return ptr;
}

struct buf {
	char *data;
	size_t size;
	size_t alloc;
};

static void buf_put(struct buf *buf, const void *data, size_t length) {
	if (length > buf->alloc - buf->size) {
		size_t nalloc = buf->alloc;
		if (nalloc == 0) {
			nalloc = 1024;
		}
		while (length > nalloc - buf->size) {
			nalloc <<= 1;
		}
		void *narr = realloc(buf->data, nalloc);
		if (narr == NULL) {
			malloc_fail(nalloc);
		}
		buf->data = narr;
		buf->alloc = nalloc;
	}
	memcpy(buf->data + buf->size, data, length);
	buf->size += length;
}

// =============================================================================

static unsigned short *gMacFromUnixData;

static noreturn void bad_unpackbits(void) {
	fputs("Error: invalid unpackbits data\n", stderr);
	exit(1);
}

static void unpackbits(void *dest, size_t destsz, const void *src,
                       size_t srcsz) {
	const unsigned char *ip = src, *ie = ip + srcsz;
	unsigned char *op = dest, *oe = op + destsz;
	while (op < oe) {
		if (ip >= ie) {
			bad_unpackbits();
		}
		int c = (signed char)*ip++;
		if (c >= 0) {
			int len = c + 1;
			if (len > ie - ip || len > oe - op) {
				bad_unpackbits();
			}
			memcpy(op, ip, len);
			op += len;
			ip += len;
		} else {
			int len = -c + 1;
			if (ip >= ie || len > oe - op) {
				bad_unpackbits();
			}
			memset(op, *ip, len);
			op += len;
			ip += 1;
		}
	}
	if (ip != ie) {
		bad_unpackbits();
	}
}

unsigned short *mac_from_unix_data(void) {
	unsigned short *ptr = gMacFromUnixData;
	if (ptr == NULL) {
		unsigned char *bytes = xmalloc(FROM_UNIX_DATALEN);
		unpackbits(bytes, FROM_UNIX_DATALEN, kFromUnixData,
		           sizeof(kFromUnixData));
		ptr = xmalloc(FROM_UNIX_DATALEN);
		for (int i = 0; i < FROM_UNIX_DATALEN / 2; i++) {
			ptr[i] = (bytes[i * 2] << 8) | bytes[i * 2 + 1];
		}
		free(bytes);
		gMacFromUnixData = ptr;
	}
	return ptr;
}

// =============================================================================

enum {
	kSrcRef = 1234,
	kDestRef = 5678,
};

static const char *gReadBuf;
static size_t gReadSize;
static size_t gReadPos;
static size_t gReadChunk;
static struct buf gWriteBuf;

int convert_read(short ref, long *count, void *data) {
	if (ref != kSrcRef) {
		fputs("Wrong ref\n", stderr);
		exit(1);
	}
	size_t amt = *count;
	size_t rem = gReadSize - gReadPos;
	if (amt > rem) {
		amt = rem;
	}
	if (gReadChunk != 0 && amt > gReadChunk) {
		amt = gReadChunk;
	}
	*count = amt;
	memcpy(data, gReadBuf + gReadPos, amt);
	gReadPos += amt;
	if (gReadPos == gReadSize) {
		return kConvertEOF;
	}
	return kConvertOK;
}

int convert_write(short ref, long count, const void *data) {
	if (ref != kDestRef) {
		fputs("Wrong ref\n", stderr);
		exit(1);
	}
	buf_put(&gWriteBuf, data, count);
	return kConvertOK;
}

// =============================================================================

enum {
	kInputSize = 64 * 1024 - 2,
};

static char *gen_input(void) {
	char *ptr = xmalloc(kInputSize);
	unsigned state = 0x12345678;
	for (int i = 0; i < kInputSize; i++) {
		// Relatively common LCG.
		state = (state * 1103515245 + 12345) & 0x7fffffff;
		ptr[i] = state >> 23;
	}
	return ptr;
}

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	int r;

	void *sbuf = xmalloc(kBufferTotalSize);
	void *dbuf = xmalloc(kBufferTotalSize);

	// Generate input.
	char *input = gen_input();

	// Convert Macintosh -> UTF-8.
	gReadBuf = input;
	gReadSize = kInputSize;
	gReadPos = 0;
	r = mac_to_unix(kSrcRef, kDestRef, sbuf, dbuf);
	if (r != 0) {
		fputs("mac_to_unix failed\n", stderr);
		return 1;
	}

	// Check that we have no CR.
	{
		const char *data = gWriteBuf.data;
		size_t size = gWriteBuf.size;
		for (size_t i = 0; i < size; i++) {
			if (data[i] == 0x0d) {
				fprintf(stderr, "Error: CR at offset %zu\n", i);
				return 1;
			}
		}
	}

	// Convert back.
	gReadBuf = gWriteBuf.data;
	gReadSize = gWriteBuf.size;
	gReadPos = 0;
	gWriteBuf = (struct buf){NULL, 0, 0};
	r = mac_from_unix(kSrcRef, kDestRef, sbuf, dbuf);
	if (r != 0) {
		fputs("mac_from_unix failed\n", stderr);
		return 1;
	}

	// Check that this is equal to original, except with LF changed to CR.
	{
		const char *data = gWriteBuf.data;
		size_t size = gWriteBuf.size;
		if (kInputSize != size) {
			fprintf(stderr, "Error: size = %zu, expect %d\n", size, kInputSize);
			return 1;
		}
		for (size_t i = 0; i < kInputSize; i++) {
			unsigned char x = input[i];
			if (x == 0x0a) {
				x = 0x0d;
			}
			unsigned char y = data[i];
			if (x != y) {
				fprintf(stderr, "Error: data[%zu] = 0x%02x, expect 0x%02x\n", i,
				        y, x);
				return 1;
			}
		}
	}

	return 0;
}

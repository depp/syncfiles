// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "lib/strbuf.h"

#include <stdlib.h>
#include <string.h>

bool StrbufAlloc(struct Strbuf *restrict b, size_t n)
{
	size_t req = n + 1;
	if (req > b->alloc) {
		size_t nalloc = (b->alloc + 16) * 3 / 2;
		if (req > nalloc) {
			nalloc = req;
		}
		char *nbuf = realloc(b->buf, nalloc);
		if (nbuf == NULL) {
			return false;
		}
		b->buf = nbuf;
		b->alloc = nalloc;
	}
	return true;
}

bool StrbufReserve(struct Strbuf *restrict b, size_t n)
{
	return StrbufAlloc(b, b->len + n);
}

bool StrbufAppendMem(struct Strbuf *restrict b, const char *s, size_t n)
{
	if (!StrbufReserve(b, n)) {
		return false;
	}
	memcpy(b->buf + b->len, s, n);
	b->buf[b->len + n] = '\0';
	b->len += n;
	return true;
}

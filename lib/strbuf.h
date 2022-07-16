// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef LIB_STRBUF_H
#define LIB_STRBUF_H
#include <stddef.h>

#include "lib/defs.h"

// A strbuf is a buffer for building a string. The structure can be
// zero-initialized.
struct Strbuf {
	char *buf;    // String data. May be NULL if strbuf is empty.
	size_t len;   // String length, not including nul terminator.
	size_t alloc; // Data reserved for string, not including terminator.
};

// Reserve enough space in the strbuf to store a string which is n bytes long.
// Also reserve enough space for the nul byte after. Return true on success.
bool StrbufAlloc(struct Strbuf *restrict b, size_t n);

// Reserve enough space in the strbuf to append n bytes to the buffer. Also
// reserve enough space for the nul byte after. Return true on success.
bool StrbufReserve(struct Strbuf *restrict b, size_t n);

// Append the given data to the buffer. Return true on success.
bool StrbufAppendMem(struct Strbuf *restrict b, const char *s, size_t n);

#endif

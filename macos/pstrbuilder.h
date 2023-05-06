// Copyright 2022-2023 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_PSTRBUILDER_H
#define MACOS_PSTRBUILDER_H

#include "lib/defs.h"

/*
Pascal String Builder
=====================

These functions operate on the Pascal-style Str255 string type, which consists
of a length byte, followed by 0-255 characters of text. Pascal string literals
can be written by putting \p at the beginning of the literal (this is a C
extension), and have an unsigned char type. C-style strings (null-terminated,
char type) are not used.

The functions here construct strings for human consumption only, not
machine-readable strings. Strings which overflow the destination buffer are
silently truncated, with an ellipsis added to the end of the buffer to show that
it’s been truncated.
*/

// A PStrBuilder is a buffer for writing a string. This should be initialized by
// setting data[0] to 0 and truncated to false.
struct PStrBuilder {
	unsigned char data[256];
	Boolean truncated;
};

// PStrInit initializes a string buffer.
void PStrInit(struct PStrBuilder *buf);

// PStrAppendChar appends a single character to the buffer.
void PStrAppendChar(struct PStrBuilder *buf, unsigned char c);

// PStrAppend appends a string to the buffer.
void PStrAppend(struct PStrBuilder *buf, const unsigned char *src);

// PStrAppendSubstitute appends a Pascal string to another string, expanding any
// ^N substitutions, where N is a positive digit, 1-9.
void PStrAppendSubstitute(struct PStrBuilder *buf, const unsigned char *src,
                          int paramcount, const unsigned char *const *params);

#endif

// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_STRUTIL_H
#define MACOS_STRUTIL_H

// Write a formatted string.
void StrFormat(unsigned char *dest, const unsigned char *msg, ...);

// Append a formatted string to another string.
//
// %% - literal %
// %d - int argument
// %S - Pascal string argument
void StrAppendFormat(unsigned char *dest, const unsigned char *msg, ...);

// Copy a Pascal string. Aborts the program if the string does not fit in the
// destination buffer.
void StrCopy(unsigned char *dest, int dest_size, unsigned char *src);

// Substitute ^1 in the string with param.
void StrSubstitute(unsigned char *str, const unsigned char *param);

#endif

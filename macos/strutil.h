// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_STRUTIL_H
#define MACOS_STRUTIL_H

// StrFormat writes a formatted string to another string, replacing it.
void StrFormat(unsigned char *dest, const unsigned char *msg, ...);

// StrAppend appends a formatted string to another string.
//
// %% - literal %
// %d - int argument
// %S - Pascal string argument
void StrAppendFormat(unsigned char *dest, const unsigned char *msg, ...);

// StrCopy copies a Pascal string. Aborts the program if the string does not fit
// in the destination buffer.
void StrCopy(unsigned char *dest, int dest_size, unsigned char *src);

// StrSubstitute substitute ^1 in the string with param.
void StrSubstitute(unsigned char *str, const unsigned char *param);

#endif

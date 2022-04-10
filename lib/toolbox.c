// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
/* toolbox.c - replacement functions for Mac OS toolbox functions

   This is used to run conversion tests on non-Mac OS systems to make
   development easier. These are not intended to make it possible to port the
   converter to non-Mac OS systems. */
#include "lib/defs.h"
#include "lib/util.h"

#include <stdlib.h>
#include <string.h>

Handle NewHandle(Size byteCount)
{
	Ptr p;
	Handle h;

	if (byteCount < 0) {
		Fatalf("NewHandle: byteCount = %ld", byteCount);
	}
	p = malloc(byteCount);
	if (byteCount > 0 && p == NULL) {
		return NULL;
	}
	h = malloc(sizeof(Ptr));
	if (h == NULL) {
		free(p);
		return NULL;
	}
	*h = p;
	return h;
}

void DisposeHandle(Handle h)
{
	if (h != NULL) {
		free(*h);
		free(h);
	}
}

Boolean ResizeHandle(Handle h, Size newSize)
{
	Ptr p;
	if (h == NULL) {
		Fatalf("ResizeHandle: h = NULL");
	}
	if (newSize < 0) {
		Fatalf("ResizeHandle: newSize = %ld", newSize);
	}
	p = realloc(*h, newSize);
	if (newSize > 0 && p == NULL) {
		return false;
	}
	*h = p;
	return true;
}

void MemClear(void *ptr, Size size)
{
	memset(ptr, 0, size);
}

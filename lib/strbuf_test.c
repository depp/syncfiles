// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "lib/strbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Segment {
	const char *buf;
	size_t size;
};

#define S "0123456789"
#define S8 S S S S S S S S

static const struct Segment kSegments[] = {
	{"abc", 3},
	{S8, 80},
	{"def", 3},
	{0, 0},
};

static const char kOut[] = "abc" S8 "def";

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	struct Strbuf s = {0, 0, 0};
	for (int i = 0; kSegments[i].buf != NULL; i++) {
		bool r = StrbufAppendMem(&s, kSegments[i].buf, kSegments[i].size);
		if (!r) {
			fputs("StrbufAppendMem returned false\n", stderr);
			exit(1);
		}
	}
	if (s.len != sizeof(kOut) - 1 || memcmp(s.buf, kOut, sizeof(kOut)) != 0) {
		fputs("Invalid result\n", stderr);
		exit(1);
	}
	free(s.buf);
	return 0;
}

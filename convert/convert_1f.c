// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.

// convert_1f.c - Forward conversion from extended ASCII to UTF-8.
#include "convert/convert.h"
#include "lib/defs.h"

struct Convert1fData {
	// Unicode characters, encoded in UTF-8, and packed MSB first. Always either
	// 2 bytes or 3 bytes.
	UInt32 chars[128];
};

struct Convert1fState {
	UInt8 lastch;
};

ErrorCode Convert1fBuild(Handle *out, Handle data, Size datasz)
{
	Handle h;
	struct Convert1fData *cvt;
	int i, n;
	UInt32 uch;
	const UInt8 *dptr, *dend;

	h = NewHandle(sizeof(struct Convert1fData));
	if (h == NULL) {
		return kErrorNoMemory;
	}
	cvt = (void *)*h;
	dptr = (void *)*data;
	dptr++;
	dend = dptr + datasz;
	for (i = 0; i < 128; i++) {
		if (dptr == dend) {
			goto bad_table;
		}
		n = *dptr++;
		if (n < 2 || 3 < n) {
			goto bad_table;
		}
		if (dend - dptr < n) {
			goto bad_table;
		}
		uch = 0;
		while (n-- > 0) {
			uch = (uch << 8) | *dptr++;
		}
		cvt->chars[i] = uch;
		if (dptr == dend) {
			goto bad_table;
		}
		n = *dptr++;
		if (dend - dptr < n) {
			goto bad_table;
		}
		dptr += n;
	}
	*out = h;
	return 0;

bad_table:
	DisposeHandle(h);
	return kErrorBadData;
}

void Convert1fRun(const void *cvtptr, LineBreakConversion lc,
                  struct ConverterState *stateptr, UInt8 **optr, UInt8 *oend,
                  const UInt8 **iptr, const UInt8 *iend)
{
	const struct Convert1fData *cvt = cvtptr;
	struct Convert1fState *state = (struct Convert1fState *)stateptr;
	UInt8 *opos = *optr;
	const UInt8 *ipos = *iptr;
	unsigned ch, lastch;
	UInt32 uch;

	ch = state->lastch;
	while (ipos < iend && oend - opos >= 3) {
		lastch = ch;
		ch = *ipos++;
		if (ch < 128) {
			if (ch == kCharLF || ch == kCharCR) {
				// Line breaks.
				if (ch == kCharLF && lastch == kCharCR) {
					if (lc == kLineBreakKeep) {
						*opos++ = ch;
					}
				} else {
					switch (lc) {
					case kLineBreakKeep:
						*opos++ = ch;
						break;
					case kLineBreakLF:
						*opos++ = kCharLF;
						break;
					case kLineBreakCR:
						*opos++ = kCharCR;
						break;
					case kLineBreakCRLF:
						*opos++ = kCharCR;
						*opos++ = kCharLF;
						break;
					}
				}
			} else {
				// ASCII characters.
				*opos++ = ch;
			}
		} else {
			// Unicode characters.
			uch = cvt->chars[ch - 128];
			if (uch > 0xffff) {
				opos[0] = uch >> 16;
				opos[1] = uch >> 8;
				opos[2] = uch;
				opos += 3;
			} else {
				opos[0] = uch >> 8;
				opos[1] = uch;
				opos += 2;
			}
		}
	}
	state->lastch = ch;

	*optr = opos;
	*iptr = ipos;
}

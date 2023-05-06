// Copyright 2022-2023 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "macos/pstrbuilder.h"

#include <string.h>

void PStrInit(struct PStrBuilder *buf)
{
	buf->data[0] = 0;
	buf->truncated = 0;
}

static void PStrAppendMem(struct PStrBuilder *buf, const unsigned char *src,
                          int slen)
{
	int dlen;

	if (slen == 0 || buf->truncated) {
		return;
	}
	dlen = buf->data[0];
	if (slen + dlen > 255) {
		// Too long, truncate.
		// FIXME: Handle other scripts:
		//   - Truncation should respect multibyte character boundaries.
		//   - Ellipsis should be taken from internationalization table.
		slen = 255 - dlen;
		memcpy(buf->data + 1 + dlen, src, slen);
		dlen += slen;
		if (dlen >= 255) {
			dlen = 254;
		}
		buf->data[1 + dlen] = 0xc9; // 0xc9 is ellipsis.
		dlen++;
		buf->data[0] = dlen;
		buf->truncated = true;
		return;
	}
	memcpy(buf->data + 1 + dlen, src, slen);
	buf->data[0] = dlen + slen;
}

void PStrAppendChar(struct PStrBuilder *buf, unsigned char c)
{
	PStrAppendMem(buf, &c, 1);
}

void PStrAppend(struct PStrBuilder *buf, const unsigned char *src)
{
	PStrAppendMem(buf, src + 1, src[0]);
}

void PStrAppendSubstitute(struct PStrBuilder *buf, const unsigned char *src,
                          int paramcount, const unsigned char *const *params)
{
	int srclen, srcpos, start, ch;
	const unsigned char *param;

	srclen = src[0];
	srcpos = 1;
	start = 1;
	while (srcpos <= srclen) {
		if (src[srcpos] == '^' && srcpos + 1 <= srclen) {
			ch = src[srcpos + 1];
			if ('1' <= ch && ch <= '9') {
				ch -= '1';
				if (ch < paramcount && (param = params[ch]) != NULL) {
					PStrAppendMem(buf, src + start, srcpos - start);
					param = params[ch];
					PStrAppendMem(buf, param + 1, param[0]);
					srcpos += 2;
					start = srcpos;
				} else {
					srcpos += 2;
				}
				continue;
			}
		}
		srcpos++;
	}
	PStrAppendMem(buf, src + start, srcpos - start);
}

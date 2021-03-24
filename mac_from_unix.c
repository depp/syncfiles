#include "convert.h"

#include <stdio.h>

int mac_from_unix(short srcRef, short destRef, void *srcBuf, void *destBuf) {
	unsigned char *op, *oe; // Output ptr, end.
	unsigned char *ip, *ie; // Input ptr, end.
	unsigned char *tmp, *curpos;
	const unsigned short *table; // Conversion table.
	unsigned entry, value, state, c, last, curvalue;
	long count, i;
	int has_eof = 0, need_input = 1, do_unput;
	int lineno = 1, r;

	table = mac_from_unix_data();
	if (table == NULL) {
		return 1;
	}

	// Initialize buffer pointers.
	ip = srcBuf;
	ie = ip;
	need_input = 1;
	op = destBuf;
	// The destination buffer has an extra byte which may be combined with a
	// diacritic.
	oe = op + kBufferBaseSize + 1;

	for (;;) {
		if (need_input || ip >= ie) {
			if (has_eof && ip == ie) {
				break;
			}

			// Save unprocessed input, move to beginning of buffer.
			count = ie - ip;
			if (count > kBufferExtraSize) {
				fputs("## Internal error\n", stderr);
				return 1;
			}
			tmp = ip;
			ip = (unsigned char *)srcBuf + kBufferExtraSize - count;
			for (i = 0; i < count; i++) {
				ip[i] = tmp[i];
			}

			// Try to fill remainder of buffer.
			count = kBufferBaseSize;

			r = convert_read(srcRef, &count, (char *)srcBuf + kBufferExtraSize);
			if (r != kConvertOK) {
				if (r == kConvertEOF) {
					has_eof = 1;
					if (count == 0) {
						break;
					}
				} else {
					return 1;
				}
			}
			ie = (unsigned char *)srcBuf + kBufferExtraSize + count;
			need_input = 0;
		}

		// If output buffer has a full chunk and an extra byte, write out the
		// chunk and keep the extra byte.
		if (op >= oe) {
			count = kBufferBaseSize;
			r = convert_write(destRef, count, destBuf);
			if (r != 0) {
				return 1;
			}
			tmp = destBuf;
			tmp[0] = tmp[kBufferBaseSize];
			op -= kBufferBaseSize;
		}

		while (ip < ie && op < oe) {
			c = *ip;
			if (c < 128) {
				ip++;
				// Note: \r = 0x0a, \n = 0x0d on old Mac compilers.
				if (c == 0x0a || c == 0x0d) {
					c = 0x0d;
					lineno++;
				}
				*op++ = c;
				last = c;
			} else {
				// Find the longest matching Unicode character.
				// curpos: ip after longest match.
				// curvalue: output character after longest match.
				state = table[last] & 0xff00;
				if (state != 0) {
					// Continue from previous character.
					do_unput = 1;
					curpos = ip;
					curvalue = last;
				} else {
					// Continue with new character.
					do_unput = 0;
					curpos = NULL;
					curvalue = 0;
				}
				tmp = ip;
				do {
					entry = table[state | *tmp++];
					state = entry & 0xff00;
					value = entry & 0xff;
					if (value != 0) {
						curpos = tmp;
						curvalue = value;
					}
				} while (state != 0 && tmp < ie);
				if (state == 0 || has_eof) {
					// We cannot consume more bytes. When state == 0, the state
					// machine will not consume any more characters. When ip ==
					// ie && has_eof, there are no more bytes available.
					if (curvalue == 0) {
						fprintf(stderr,
						        "## Error: line %d: invalid character\n",
						        lineno);
						return 1;
					}
					ip = curpos;
					if (do_unput) {
						op--;
					}
					*op++ = curvalue;
					last = 0;
				} else {
					// We can consume more bytes. Get more, and come back.
					need_input = 1;
					break;
				}
			}
		}
	}

	// Write remainder of output buffer.
	if (op != destBuf) {
		count = op - (unsigned char *)destBuf;
		r = convert_write(destRef, count, destBuf);
		if (r != 0) {
			return 1;
		}
	}

	return 0;
}

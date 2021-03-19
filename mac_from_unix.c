#include "defs.h"
#include "mac_from_unix_data.h"

#include <Quickdraw.h>

#include <stdio.h>

static unsigned short *gFromUnixData;

static void print_uerr(const unsigned char *start, const unsigned char *end) {
	const unsigned char *ptr;
	int lineno = 1, colno = 0;
	for (ptr = start; ptr != end; ptr++) {
		colno++;
		// Note: \r != 0x0d, \n != 0x0a on old Mac compilers.
		if (*ptr == 0x0a || *ptr == 0x0d) {
			lineno++;
			colno = 0;
		}
	}
	fprintf(stderr, "## Error: line %d, column %d: invalid character\n", lineno,
	        colno);
}

int mac_from_unix(unsigned char **outptr, unsigned char *outend,
                  const unsigned char **inptr, const unsigned char *inend) {
	unsigned char *op = *outptr;
	const unsigned char *ip = *inptr, *curpos;
	const unsigned short *table;
	unsigned entry, value, state, c, last, curvalue;

	table = gFromUnixData;
	if (table == NULL) {
		print_err("table not loaded");
		return 1;
	}
	last = 0;
	while (ip < inend && op < outend) {
		c = *ip;
		if (c < 128) {
			// Note: \r != 0x0d, \n != 0x0a on old Mac compilers.
			if (c == 0x0a) {
				c = 0x0d;
			}
			if (op == outend) {
				break;
			}
			*op = c;
			last = c;
			ip++;
			op++;
		} else {
			// Find the longest matching Unicode character.
			state = table[last] & 0xff00;
			if (state != 0) {
				// Continue from previous character.
				op--;
				curpos = ip;
				curvalue = last;
			} else {
				// Continue from current character.
				curpos = NULL;
				curvalue = 0;
			}
			do {
				entry = table[state | *ip++];
				state = entry & 0xff00;
				value = entry & 0xff;
				if (value != 0) {
					curpos = ip;
					curvalue = value;
				}
			} while (state != 0 && ip < inend);
			if (curvalue == 0) {
				print_uerr(*outptr, op);
				*outptr = op;
				*inptr = ip;
				return 1;
			}
			ip = curpos;
			*op++ = curvalue;
			last = 0;
		}
	}
	*outptr = op;
	*inptr = ip;
	return 0;
}

int mac_from_unix_init(void) {
	Ptr ptr, src, dest;
	OSErr err;

	if (gFromUnixData != NULL) {
		return 0;
	}
	ptr = NewPtr(FROM_UNIX_DATALEN);
	err = MemError();
	if (err != 0) {
		print_errcode(err, "out of memory");
		return 1;
	}
	src = (void *)kFromUnixData;
	dest = ptr;
	UnpackBits(&src, &dest, FROM_UNIX_DATALEN);
	gFromUnixData = (void *)ptr;
	return 0;
}

void mac_from_unix_term(void) {
	if (gFromUnixData != NULL) {
		DisposePtr((void *)gFromUnixData);
		gFromUnixData = NULL;
	}
}

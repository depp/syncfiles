#include "convert.h"

int convert_line_endings(short srcRef, short destRef, void *buf,
                         unsigned char srcEnding, unsigned char destEnding) {
	unsigned char *ptr, *end;
	long count;
	int r, r2;

	do {
		count = kBufferBaseSize;
		r = convert_read(srcRef, &count, buf);
		if (r == kConvertError) {
			return 1;
		}
		for (ptr = buf, end = ptr + count; ptr != end; ptr++) {
			if (*ptr == srcEnding) {
				*ptr = destEnding;
			}
		}
		r2 = convert_write(destRef, count, buf);
		if (r2 != kConvertOK) {
			return 1;
		}
	} while (r != kConvertEOF);
	return 0;
}

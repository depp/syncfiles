#include "convert.h"

int copy_data(short srcRef, short destRef, void *buf) {
	long count;
	int r, r2;

	do {
		count = kBufferBaseSize;
		r = convert_read(srcRef, &count, buf);
		if (r == kConvertError) {
			return 1;
		}
		r2 = convert_write(destRef, count, buf);
		if (r2 != kConvertOK) {
			return 1;
		}
	} while (r != kConvertEOF);
	return 0;
}

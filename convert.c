// convert.c - Conversion helper functions.
#include "convert.h"

#include "defs.h"
#include "mac_from_unix_data.h"

#include <CursorCtl.h>
#include <Files.h>
#include <MacErrors.h>
#include <Quickdraw.h>

int convert_read(short ref, long *count, void *data) {
	OSErr err;

	SpinCursor(1);
	err = FSRead(ref, count, data);
	switch (err) {
	case noErr:
		return kConvertOK;
	case eofErr:
		return kConvertEOF;
	default:
		print_errcode(err, "could not read source file");
		return kConvertError;
	}
}

int convert_write(short ref, long count, const void *data) {
	OSErr err;

	SpinCursor(1);
	err = FSWrite(ref, &count, data);
	if (err == noErr) {
		return kConvertOK;
	}
	print_errcode(err, "could not write temp file");
	return kConvertError;
}

static unsigned short *gFromUnixData;

// Get the table for converting from Unix to Macintosh.
unsigned short *mac_from_unix_data(void) {
	Ptr ptr, src, dest;

	if (gFromUnixData != NULL) {
		return gFromUnixData;
	}
	ptr = NewPtr(FROM_UNIX_DATALEN);
	if (ptr == NULL) {
		print_memerr(FROM_UNIX_DATALEN);
		return NULL;
	}
	src = (void *)kFromUnixData;
	dest = ptr;
	UnpackBits(&src, &dest, FROM_UNIX_DATALEN);
	gFromUnixData = (void *)ptr;
	return gFromUnixData;
}

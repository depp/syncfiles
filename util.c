#include "defs.h"

#include <MacErrors.h>
#include <MacMemory.h>
#include <MacTypes.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void print_err(const char *msg, ...) {
	va_list ap;
	fputs("## Error: ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);
}

// Map from OSErr codes to messages. As a heuristic, this should include error
// codes caused by toolbox calls in this program which are readily understood by
// the user. It can exclude error codes that are likely to indicate a
// programming error.
struct error_message {
	OSErr err;
	const char *msg;
};
#define E(e, m) \
	{ e, m "\0" #e }
static const struct error_message kErrorMessages[] = {
	E(dirFulErr, "directory full"),                                   // -33
	E(dskFulErr, "disk full"),                                        // -34
	E(ioErr, "I/O error"),                                            // -36
	E(bdNamErr, "bad name"),                                          // -37
	E(fnfErr, "file not found"),                                      // -43
	E(wPrErr, "disk is write-protected"),                             // -44
	E(fLckdErr, "file is locked"),                                    // -45
	E(vLckdErr, "volume is locked"),                                  // -46
	E(fBsyErr, "file is busy"),                                       // -47
	E(dupFNErr, "destination already exists"),                        // -48
	E(opWrErr, "file already open for writing"),                      // -49
	E(paramErr, "parameter error"),                                   // -50
	E(permErr, "cannot write to locked file"),                        // -54
	E(dirNFErr, "directory not found"),                               // -120
	E(wrgVolTypErr, "not an HFS volume"),                             // -123
	E(diffVolErr, "files on different volumes"),                      // -1303
	E(afpAccessDenied, "user does not have access privileges (AFP)"), // -5000
	E(afpObjectTypeErr,
      "file/directory specified where directory/file expected"), // -5025
	E(afpSameObjectErr, "objects are the same"),                 // -5038
};
#undef E

// Return the error message for a Macintosh toolbox error code. Returns NULL if
// the error is not known.
static const char *mac_strerror(OSErr err) {
	int i, n = sizeof(kErrorMessages) / sizeof(*kErrorMessages);
	for (i = 0; i < n; i++) {
		if (kErrorMessages[i].err == err) {
			return kErrorMessages[i].msg;
		}
	}
	return NULL;
}

void print_errcode(OSErr err, const char *msg, ...) {
	va_list ap;
	const char *emsg;

	fputs("## Error: ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	emsg = mac_strerror(err);
	if (emsg != NULL) {
		fprintf(stderr, ": %s (%d)\n", emsg, err);
	} else {
		fprintf(stderr, ": err=%d\n", err);
	}
}

void print_memerr(unsigned long size) {
	OSErr err;

	err = MemError();
	print_errcode(err, "out of memory; size=%lu", size);
}

void log_call(OSErr err, const char *function) {
	const char *emsg;

	if (err == 0) {
		fprintf(stderr, "## %s: noErr\n", function);
		return;
	}
	emsg = mac_strerror(err);
	if (emsg != NULL) {
		emsg += strlen(emsg) + 1;
		fprintf(stderr, "## %s: %s (%d)\n", function, emsg, err);
	} else {
		fprintf(stderr, "## %s: %d\n", function, err);
	}
}

int c2pstr(Str255 ostr, const char *istr) {
	size_t n = strlen(istr);
	if (n > 255) {
		print_err("path too long: %s", istr);
		return 1;
	}
	ostr[0] = n;
	memcpy(ostr + 1, istr, n);
	memset(ostr + 1 + n, 0, 255 - n);
	return 0;
}

void p2cstr(char *ostr, const unsigned char *istr) {
	unsigned len = istr[0];
	memcpy(ostr, istr + 1, len);
	ostr[len] = '\0';
}

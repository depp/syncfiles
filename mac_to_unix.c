#include "convert.h"

// Table that converts Macintosh Roman characters to UTF-8, and CR to LF.
static const unsigned short kToUnixTable[256] = {
	0,     1,   2,    3,    4,    5,    6,     7,     8,    9,    10,   11,
	12,    10,  14,   15,   16,   17,   18,    19,    20,   21,   22,   23,
	24,    25,  26,   27,   28,   29,   30,    31,    32,   33,   34,   35,
	36,    37,  38,   39,   40,   41,   42,    43,    44,   45,   46,   47,
	48,    49,  50,   51,   52,   53,   54,    55,    56,   57,   58,   59,
	60,    61,  62,   63,   64,   65,   66,    67,    68,   69,   70,   71,
	72,    73,  74,   75,   76,   77,   78,    79,    80,   81,   82,   83,
	84,    85,  86,   87,   88,   89,   90,    91,    92,   93,   94,   95,
	96,    97,  98,   99,   100,  101,  102,   103,   104,  105,  106,  107,
	108,   109, 110,  111,  112,  113,  114,   115,   116,  117,  118,  119,
	120,   121, 122,  123,  124,  125,  126,   127,   196,  197,  199,  201,
	209,   214, 220,  225,  224,  226,  228,   227,   229,  231,  233,  232,
	234,   235, 237,  236,  238,  239,  241,   243,   242,  244,  246,  245,
	250,   249, 251,  252,  8224, 176,  162,   163,   167,  8226, 182,  223,
	174,   169, 8482, 180,  168,  8800, 198,   216,   8734, 177,  8804, 8805,
	165,   181, 8706, 8721, 8719, 960,  8747,  170,   186,  937,  230,  248,
	191,   161, 172,  8730, 402,  8776, 8710,  171,   187,  8230, 160,  192,
	195,   213, 338,  339,  8211, 8212, 8220,  8221,  8216, 8217, 247,  9674,
	255,   376, 8260, 8364, 8249, 8250, 64257, 64258, 8225, 183,  8218, 8222,
	8240,  194, 202,  193,  203,  200,  205,   206,   207,  204,  211,  212,
	63743, 210, 218,  219,  217,  305,  710,   732,   175,  728,  729,  730,
	184,   733, 731,  711,
};

int mac_to_unix(short srcRef, short destRef, void *srcBuf, void *destBuf) {
	unsigned char *op, *oe, *tmp; // Output ptr, end.
	const unsigned char *ip, *ie; // Input ptr, end.
	unsigned cp;                  // Code point.
	int r;
	long count;
	int has_eof = 0;

	// Initialize buffer pointers.
	ip = srcBuf;
	ie = ip;
	op = destBuf;
	oe = op + kBufferBaseSize;

	for (;;) {
		// If input buffer is consumed, read more.
		if (ip >= ie) {
			if (has_eof) {
				break;
			}
			count = kBufferBaseSize;
			r = convert_read(srcRef, &count, srcBuf);
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
			ip = srcBuf;
			ie = ip + count;
		}

		// If output buffer has a full chunk, write it out.
		if (op >= oe) {
			count = kBufferBaseSize;
			r = convert_write(destRef, count, destBuf);
			if (r != 0) {
				return 1;
			}
			tmp = destBuf;
			tmp[0] = tmp[kBufferBaseSize];
			tmp[1] = tmp[kBufferBaseSize + 1];
			op -= kBufferBaseSize;
		}

		// Convert as much as possible. Note that the "extra" past the end of
		// the destination buffer may be used, just to simplify bounds checking.
		while (ip < ie && op < oe) {
			cp = kToUnixTable[*ip++];
			if (cp < 0x80) {
				op[0] = cp;
				op += 1;
			} else if (cp < 0x400) {
				op[0] = (cp >> 6) | 0xc0;
				op[1] = (cp & 0x3f) | 0x80;
				op += 2;
			} else {
				op[0] = (cp >> 12) | 0xe0;
				op[1] = ((cp >> 6) & 0x3f) | 0x80;
				op[2] = (cp & 0x3f) | 0x80;
				op += 3;
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

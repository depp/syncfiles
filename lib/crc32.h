#ifndef CRC32_H
#define CRC32_H

#include "lib/defs.h"

/* Incrementally calculate a CRC32. This is the same CRC32 used by Gzip. */
UInt32 CRC32Update(UInt32 crc, const void *ptr, Size size);

#endif

// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef LIB_CRC32_H
#define LIB_CRC32_H

#include "lib/defs.h"

/* Incrementally calculate a CRC32. This is the same CRC32 used by Gzip. */
UInt32 CRC32Update(UInt32 crc, const void *ptr, Size size);

#endif

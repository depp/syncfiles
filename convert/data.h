// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef CONVERT_DATA_H
#define CONVERT_DATA_H
// data.h - charmap data, not used for classic Mac OS builds
#include "lib/defs.h"

// Get the ID of the given character map. Return NULL if no such character map
// exists.
const char *CharmapID(int cmap);

// Get the human-readable name fo the given character map. Return NULL if no
// such character map exists.
const char *CharmapName(int cmap);

// Conversion table data.
struct CharmapData {
	const UInt8 *ptr;
	Size size;
};

// Get the conversion table data for the given charmap. Returns an empty buffer
// with a NULL pointer if the character map does not exist or if no conversion
// table exists for that character map.
struct CharmapData CharmapData(int cmap);

#endif

// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_TEMPFILE_H
#define MACOS_TEMPFILE_H

// Create a FSSpec for a temporary file on the given volume. Does not actually
// create the file.
OSErr MakeTempFile(short vRefNum, FSSpec *spec);

#endif

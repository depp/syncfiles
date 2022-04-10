// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_PATH_H
#define MACOS_PATH_H

// Get the full path to a directory.
OSErr GetDirPath(FSSpec *spec, int *pathLength, Handle *path);

#endif

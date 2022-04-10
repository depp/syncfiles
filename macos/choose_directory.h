// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_CHOOSE_DIRECTORY_H
#define MACOS_CHOOSE_DIRECTORY_H

// Present a dialog box to choose a directory. Returns true and initializes
// directory if a directory is selected.
Boolean ChooseDirectory(FSSpec *directory);

#endif

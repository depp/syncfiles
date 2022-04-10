// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef MACOS_MAIN_H
#define MACOS_MAIN_H

// Menu item IDs.
enum {
	iApple_Info = 1,

	iFile_New = 1,
	iFile_Open = 2,
	iFile_Close = 4,
	iFile_Save = 5,
	iFile_SaveAs = 6,
	iFile_Revert = 7,
	iFile_Quit = 9,
};

// Quit the application.
void QuitApp(void);

#endif

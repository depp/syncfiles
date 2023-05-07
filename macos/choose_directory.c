// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "macos/choose_directory.h"

#include "macos/error.h"
#include "macos/resources.h"

#include <string.h>

typedef enum {
	kCDNone,
	kCDChild,
	kCDCurrent,
} ChooseDirStatus;

struct ChooseDirReply {
	ChooseDirStatus status;
	FSSpec *directory;
};

// FIXME: Don't use pascal on PowerMac.
static pascal Boolean ChooseDirectoryFileFilter(CInfoPBPtr pb,
                                                void *yourDataPtr)
{
	(void)yourDataPtr;
	return (pb->hFileInfo.ioFlAttrib & ioDirMask) == 0;
}

enum {
	kCDItemChoose = 10,
	kCDItemChooseCurrent = 11,
};

static void SelectCurrentDirectory(struct ChooseDirReply *reply)
{
	CInfoPBRec ci;
	OSStatus err;
	Str63 name;
	FSSpec *directory;

	memset(&ci, 0, sizeof(ci));
	ci.dirInfo.ioNamePtr = name;
	ci.dirInfo.ioVRefNum = -LMGetSFSaveDisk();
	ci.dirInfo.ioFDirIndex = -1;
	ci.dirInfo.ioDrDirID = LMGetCurDirStore();
	err = PBGetCatInfoSync(&ci);
	if (err != 0) {
		ShowError(kErrNone, kErrNone, err, NULL);
		return;
	}
	directory = reply->directory;
	directory->vRefNum = ci.dirInfo.ioVRefNum;
	directory->parID = ci.dirInfo.ioDrParID;
	if (name[0] >= sizeof(directory->name)) {
		EXIT_INTERNAL();
	}
	memcpy(directory->name, name, name[0] + 1);
	reply->status = kCDCurrent;
}

static pascal short ChooseDirectoryDialogHook(short item, DialogRef theDialog,
                                              void *yourDataPtr)
{
	struct ChooseDirReply *reply;
	if (GetWRefCon((WindowRef)theDialog) != sfMainDialogRefCon) {
		return item;
	}
	reply = yourDataPtr;
	switch (item) {
	case kCDItemChoose:
		reply->status = kCDChild;
		return sfItemCancelButton;
	case kCDItemChooseCurrent:
		SelectCurrentDirectory(yourDataPtr);
		return sfItemCancelButton;
	default:
		return item;
	}
}

Boolean ChooseDirectory(FSSpec *directory)
{
	struct ChooseDirReply cdreply;
	FileFilterYDUPP fileFilter;
	DlgHookYDUPP dlgHook;
	StandardFileReply sfreply;
	Point where;

	cdreply.status = kCDNone;
	cdreply.directory = directory;
	fileFilter = NewFileFilterYDProc(ChooseDirectoryFileFilter);
	ASSERT(fileFilter != NULL);
	dlgHook = NewDlgHookYDProc(ChooseDirectoryDialogHook);
	ASSERT(dlgHook != NULL);
	where.v = -1;
	where.h = -1;
	CustomGetFile(fileFilter, -1, NULL, &sfreply, rDLOG_OpenFolder, where,
	              dlgHook, NULL /* filterProc */, 0, NULL, &cdreply);
	// DisposeFileFilterYDUPP(fileFilter);
	// DisposeDlgHookYDUPP(dlgHook);
	switch (cdreply.status) {
	case kCDChild:
		BlockMoveData(&sfreply.sfFile, directory, sizeof(FSSpec));
		return true;
	case kCDCurrent:
		return true;
	default:
		return false;
	}
}

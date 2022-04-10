// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "project.h"

#include "choose_directory.h"
#include "crc32.h"
#include "error.h"
#include "main.h"
#include "path.h"
#include "resources.h"
#include "tempfile.h"

#include <Files.h>
#include <MacMemory.h>
#include <MacWindows.h>

#include <string.h>

/*
Project format:

Header:
byte[32]	magic
uint32		version
uint32		file size
uint32		data crc32
uint32		chunk count
ckinfo[]	chunk info
byte[]		chunk data

Chunk info:
byte[8]		chunk name
uint32		byte offset
uint32		byte length

Chunks: (names zero-padded)
"s.alias"	source directory alias
"d.alias"	destination directory alias

Chunks are aligned to 16 byte boundaries, and the file is padded to a 16 byte
boundary. This is just to make it easier to read hexdumps.
*/

// clang-format off

// Magic cookie that identifies project files.
static const kMagic[32] = {
	// Identify the file type.
	'S', 'y', 'n', 'c', 'F', 'i', 'l', 'e', 's', ' ',
	'P', 'r', 'o', 'j', 'e', 'c', 't',
	// Detect newline conversions.
	0, 10, 0, 13, 0, 13, 10, 0,
	// Detect encoding conversions (infinity in Roman and UTF-8).
	0xb0, 0, 0xe2, 0x88, 0x9e, 0, 0,
};

// clang-format on

static const kChunkAlias[2][8] = {
	{'l', '.', 'a', 'l', 'i', 'a', 's', 0},
	{'r', '.', 'a', 'l', 'i', 'a', 's', 0},
};

struct ProjectHeader {
	UInt8 magic[32];
	UInt32 version;
	UInt32 size;
	UInt32 crc32;
	UInt32 chunkCount;
};

struct ProjectChunk {
	UInt8 name[8];
	UInt32 offset;
	UInt32 size;
};

// Dimensions of controls.
enum {
	kVBorder = 10,
	kHBorder = 10,
	kDirVSize = 62,
};

enum {
	// Base ID for aliases.
	rAliasBase = 128,
};

enum {
	kControlChooseDirLocal,
	kControlChooseDirRemote,
};

// FIXME: this is redundant.
enum {
	kDirLocal,
	kDirRemote,
};

struct ProjectDir {
	int pathLength;
	Handle path;
	AliasHandle alias;
};

// A synchronization project.
struct Project {
	int windowWidth;
	int isActive;

	// File reference to the project file, or fileRef == 0 if file not saved.
	short fileRef;
	FSSpec fileSpec;

	struct ProjectDir dirs[2];
};

void ProjectNew(void)
{
	ProjectHandle project;
	WindowRef window;
	ControlRef control;
	struct Project *projectp;
	int i, windowWidth, controlWidth;

	project = (ProjectHandle)NewHandle(sizeof(struct Project));
	if (project == NULL) {
		EXIT_ASSERT(NULL);
	}
	window = GetNewCWindow(rWIND_Project, NULL, (WindowPtr)-1);
	if (window == NULL) {
		EXIT_ASSERT(NULL);
	}
	windowWidth = window->portRect.right - window->portRect.left;
	SetWRefCon(window, (long)project);
	projectp = *project;
	memset(projectp, 0, sizeof(*projectp));
	projectp->windowWidth = windowWidth;

	for (i = 0; i < 2; i++) {
		control = GetNewControl(rCNTL_ChooseFolder, window);
		if (control == NULL) {
			EXIT_ASSERT(NULL);
		}
		controlWidth =
			(*control)->contrlRect.right - (*control)->contrlRect.left;
		MoveControl(control, windowWidth - controlWidth - kHBorder,
		            kVBorder + kDirVSize * i + 22);
		SetControlReference(control, kControlChooseDirLocal + i);
	}

	ShowWindow(window);
}

void ProjectAdjustMenus(ProjectHandle project, MenuHandle fileMenu)
{
	(void)project;
	if (fileMenu != NULL) {
		EnableItem(fileMenu, iFile_Close);
		EnableItem(fileMenu, iFile_Save);
		EnableItem(fileMenu, iFile_SaveAs);
		EnableItem(fileMenu, iFile_Revert);
	}
}

static void ProjectClose(WindowRef window, ProjectHandle project)
{
	struct Project *projectp;
	int i;

	KillControls(window);
	DisposeWindow(window);
	HLock((Handle)project);
	projectp = *project;
	if (projectp->fileRef != 0) {
		FSClose(projectp->fileRef);
	}
	for (i = 0; i < 2; i++) {
		if (projectp->dirs[i].path != NULL) {
			DisposeHandle(projectp->dirs[i].path);
		}
	}
	DisposeHandle((Handle)project);
}

#define kHeaderSize 48
#define kChunkInfoSize 16
#define kChunkCount 2

#define ALIGN(x) (((x) + 15) + ~(UInt32)15)

struct ProjectData {
	Handle data;
	Size size;
};

static OSErr ProjectMarshal(ProjectHandle project, struct ProjectData *datap)
{
	struct ProjectChunk ckInfo[kChunkCount];
	Handle ckData[kChunkCount];
	UInt32 size, pos, ckOff, ckSize;
	Handle h;
	int i;
	struct ProjectHeader *hdr;

	size = kHeaderSize + kChunkCount * kChunkInfoSize;
	for (i = 0; i < 2; i++) {
		h = (Handle)(*project)->dirs[i].alias;
		ckSize = GetHandleSize(h);
		memcpy(ckInfo[i].name, kChunkAlias[i], 8);
		ckInfo[i].offset = size;
		ckInfo[i].size = ckSize;
		ckData[i] = h;
		size = ALIGN(size + ckSize);
	}

	h = NewHandle(size);
	if (h == NULL) {
		return MemError();
	}
	hdr = (void *)*h;
	memcpy(hdr->magic, kMagic, sizeof(kMagic));
	hdr->version = 0x10000;
	hdr->size = size;
	hdr->chunkCount = kChunkCount;
	memcpy(*h + kHeaderSize, ckInfo, sizeof(ckInfo));
	pos = kHeaderSize + kChunkCount * kChunkInfoSize;
	for (i = 0; i < kChunkCount; i++) {
		ckOff = ckInfo[i].offset;
		ckSize = ckInfo[i].size;
		memset(*h + pos, 0, ckOff - pos);
		memcpy(*h + ckOff, *ckData[i], ckSize);
		pos = ckOff + ckSize;
	}
	memcpy(*h + pos, 0, size - pos);

	datap->data = h;
	datap->size = size;
	return noErr;
}

// Prompt the user for a location to save the project.
static void ProjectSaveAs(WindowRef window, ProjectHandle project)
{
	// See listing 1-12 in IM: Files. This has some differences.
	Str255 name;
	StandardFileReply reply;
	short refNum;
	struct ProjectData data;
	struct Project *projectp;
	OSErr err;
	Boolean hasfile;
	ParamBlockRec pb;
	struct ErrorParams errp;
	long size;

	data.data = NULL;
	refNum = 0;
	hasfile = FALSE;

	GetWTitle(window, name);
	StandardPutFile("\pSave project as:", name, &reply);
	if (!reply.sfGood) {
		return;
	}

	err = ProjectMarshal(project, &data);
	if (err != noErr) {
		goto error;
	}

	// Delete file if replacing. This way, we get a new creation date (makes
	// sense) and the correct creator and type codes.
	if (reply.sfReplacing) {
		err = FSpDelete(&reply.sfFile);
		if (err != noErr) {
			goto error;
		}
	}

	// Write out the new file.
	err = FSpCreate(&reply.sfFile, kCreator, kTypeProject, reply.sfScript);
	if (err != noErr) {
		goto error;
	}
	hasfile = TRUE;
	err = FSpOpenDF(&reply.sfFile, fsWrPerm, &refNum);
	if (err != noErr) {
		goto error;
	}
	size = data.size;
	err = FSWrite(refNum, &size, *data.data);
	if (err != noErr) {
		goto error;
	}
	DisposeHandle(data.data);
	data.data = NULL;
	memset(&pb, 0, sizeof(pb));
	pb.fileParam.ioFRefNum = refNum;
	PBFlushFile(&pb, FALSE);
	err = pb.fileParam.ioResult;
	if (err != noErr) {
		goto error;
	}

	// Done writing, update the window and project.
	SetWTitle(window, reply.sfFile.name);
	projectp = *project;
	if (projectp->fileRef != 0) {
		FSClose(projectp->fileRef);
	}
	projectp->fileRef = refNum;
	projectp->fileSpec = reply.sfFile;
	return;

error:
	if (data.data != NULL) {
		DisposeHandle(data.data);
	}
	if (refNum != 0) {
		FSClose(refNum);
	}
	if (hasfile) {
		FSpDelete(&reply.sfFile);
	}
	memset(&errp, 0, sizeof(errp));
	errp.err = kErrCouldNotSaveProject;
	errp.osErr = err;
	errp.strParam = reply.sfFile.name;
	ShowError(&errp);
}

static void ProjectSaveImpl(WindowRef window, ProjectHandle project)
{
	struct ProjectData data;
	short refNum;
	FSSpec temp;
	OSErr err;
	struct ErrorParams errp;
	Boolean hasfile;
	long size;

	(void)window;

	data.data = NULL;
	refNum = 0;
	hasfile = FALSE;

	err = ProjectMarshal(project, &data);
	if (err != noErr) {
		goto error;
	}
	err = MakeTempFile((*project)->fileSpec.vRefNum, &temp);
	if (err != noErr) {
		goto error;
	}
	err = FSpCreate(&temp, 'trsh', 'trsh', smSystemScript);
	if (err != noErr) {
		goto error;
	}
	hasfile = TRUE;
	err = FSpOpenDF(&temp, fsWrPerm, &refNum);
	if (err != noErr) {
		goto error;
	}
	size = data.size;
	err = FSWrite(refNum, &size, *data.data);
	if (err != noErr) {
		goto error;
	}
	DisposeHandle(data.data);
	data.data = NULL;
	err = FSClose(refNum);
	refNum = 0;
	if (err != noErr) {
		goto error;
	}
	err = FSpExchangeFiles(&temp, &(*project)->fileSpec);
	if (err != noErr) {
		goto error;
	}
	FSpDelete(&temp);
	return;

error:
	if (data.data) {
		DisposeHandle(data.data);
	}
	if (refNum != 0) {
		FSClose(refNum);
	}
	if (hasfile) {
		FSpDelete(&temp);
	}
	memcpy(&temp.name, &(*project)->fileSpec.name, sizeof(temp.name));
	memset(&errp, 0, sizeof(errp));
	errp.err = kErrCouldNotSaveProject;
	errp.osErr = err;
	errp.strParam = temp.name;
	ShowError(&errp);
}

// Save the project to disk in response to a Save command.
static void ProjectSave(WindowRef window, ProjectHandle project)
{
	if ((*project)->fileRef == 0) {
		ProjectSaveAs(window, project);
	} else {
		ProjectSaveImpl(window, project);
	}
}

void ProjectCommand(WindowRef window, ProjectHandle project, int menuID,
                    int item)
{
	switch (menuID) {
	case rMENU_File:
		switch (item) {
		case iFile_Close:
			ProjectClose(window, project);
			break;
		case iFile_Save:
			ProjectSave(window, project);
			break;
		case iFile_SaveAs:
			ProjectSaveAs(window, project);
			break;
		}
		break;
	}
}

void ProjectUpdate(WindowRef window, ProjectHandle project)
{
	struct Project *projectp;
	Rect rect;
	Handle text;
	int i;

	HLock((Handle)project);

	projectp = *project;
	rect = window->portRect;
	EraseRect(&rect);
	MoveTo(0, kVBorder + kDirVSize - 10);
	LineTo(projectp->windowWidth, kVBorder + kDirVSize - 10);

	for (i = 0; i < 2; i++) {
		MoveTo(kHBorder, kVBorder + kDirVSize * i + 12);
		DrawString(i == 0 ? "\pLocal Folder" : "\pRemote Folder");
		text = projectp->dirs[i].path;
		if (text != NULL) {
			HLock(text);
			MoveTo(kHBorder, kVBorder + kDirVSize * i + 36);
			DrawText(*text, 0, projectp->dirs[i].pathLength);
			HUnlock(text);
		}
	}
	UpdateControls(window, window->visRgn);

	HUnlock((Handle)project);
}

void ProjectActivate(WindowRef window, ProjectHandle project, int isActive)
{
	struct Project *projectp;

	projectp = *project;
	if (projectp->isActive != isActive) {
		projectp->isActive = isActive;
		InvalRect(&window->portRect);
	}
}

static void ProjectChooseDir(WindowRef window, ProjectHandle project,
                             int whichDir)
{
	struct Project *projectp;
	struct ProjectDir *dirp;
	FSSpec directory;
	Handle path;
	int pathLength;
	OSErr err;

	if (ChooseDirectory(&directory)) {
		err = GetDirPath(&directory, &pathLength, &path);
		if (err != noErr) {
			ExitErrorOS(kErrUnknown, err);
		}
		projectp = *project;
		dirp = &projectp->dirs[whichDir];
		if (dirp->path != NULL) {
			DisposeHandle(dirp->path);
		}
		dirp->pathLength = pathLength;
		dirp->path = path;
		InvalRect(&window->portRect);
	}
}

void ProjectMouseDown(WindowRef window, ProjectHandle project,
                      EventRecord *event)
{
	Point mouse;
	GrafPtr savedPort;
	ControlRef control;
	ControlPartCode part;
	long reference;

	GetPort(&savedPort);
	SetPort(window);
	mouse = event->where;
	GlobalToLocal(&mouse);
	part = FindControl(mouse, window, &control);
	switch (part) {
	case kControlButtonPart:
		reference = GetControlReference(control);
		if (TrackControl(control, mouse, NULL) != 0) {
			switch (reference) {
			case kControlChooseDirLocal:
				ProjectChooseDir(window, project, kDirLocal);
				break;
			case kControlChooseDirRemote:
				ProjectChooseDir(window, project, kDirRemote);
				break;
			}
		}
		break;
	}
	SetPort(savedPort);
}

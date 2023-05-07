// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "macos/project.h"

#include "lib/crc32.h"
#include "macos/choose_directory.h"
#include "macos/error.h"
#include "macos/main.h"
#include "macos/path.h"
#include "macos/resources.h"
#include "macos/tempfile.h"

#include <Aliases.h>
#include <MacMemory.h>
#include <MacWindows.h>

#include <string.h>

/*
Project format:

Header:
byte[32]	magic
uint32		version
uint32		header length
uint32		header crc32
uint32		chunk count
ckinfo[]	chunk info
byte[]		chunk data

Chunk info:
uint32		chunk ID
uint32		byte offset
uint32		byte length
uint32		chunk crc32

Chunks: (names zero-padded)
"LOCA"	local directory alias
"LOCP"	local directory path
"REMA"	destination directory alias
"REMP"	destination directory path

Chunks are aligned to 16 byte boundaries, and the file is padded to a 16 byte
boundary. This is just to make it easier to read hexdumps.
*/

#define kVersion 0x10000

#define kMaxChunkCount 32
#define kMaxChunkSize (4 * 1024)

// clang-format off

// Magic cookie that identifies project files.
static const unsigned char kMagic[32] = {
	// Identify the file type.
	'S', 'y', 'n', 'c', 'F', 'i', 'l', 'e', 's',
	' ', 'P', 'r', 'o', 'j', 'e', 'c', 't',
	// Detect newline conversions.
	0, 10, 0, 13, 0, 13, 10, 0,
	// Detect encoding conversions (infinity in Roman and UTF-8).
	0xb0, 0, 0xe2, 0x88, 0x9e, 0, 0
};

// clang-format on

struct ProjectHeader {
	UInt8 magic[32];
	UInt32 version;
	UInt32 size;
	UInt32 crc32;
	UInt32 chunkCount;
};

struct ProjectChunk {
	UInt32 id;
	UInt32 offset;
	UInt32 size;
	UInt32 crc32;
};

static const UInt32 kProjectAliasChunks[2] = {'LOCA', 'REMA'};
static const UInt32 kProjectPathChunks[2] = {'LOCP', 'REMP'};

// Dimensions of controls.
enum {
	kVBorder = 10,
	kHBorder = 10,
	kDirVSize = 62,
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

	// File reference to the project file, or fileRef == 0 otherwise.
	short fileRef;
	// Location of the project file, if vRefNum != 0. In a possible edge case,
	// the fileSpec may be set even when fileRef is 0. This may happen if you
	// save the project to a volume which does not support FSpExchangeFiles(),
	// and the operation fails.
	FSSpec fileSpec;
	// Script code for the filename.
	ScriptCode fileScript;

	struct ProjectDir dirs[2];
};

// ProjectNewObject creates a new empty Project object.
static ProjectHandle ProjectNewObject(void)
{
	ProjectHandle project;

	project = (ProjectHandle)NewHandle(sizeof(struct Project));
	if (project == NULL) {
		ShowMemError();
		return NULL;
	}
	memset(*project, 0, sizeof(struct Project));
	return project;
}

// ProjectDelete deletes a Project object. It does not free any associated
// windows.
static void ProjectDelete(ProjectHandle project)
{
	Handle h;
	int i;
	short fileRef;

	fileRef = (*project)->fileRef;
	if (fileRef != 0) {
		FSClose(fileRef);
	}
	for (i = 0; i < 2; i++) {
		h = (*project)->dirs[i].path;
		if (h != NULL) {
			DisposeHandle(h);
		}
		h = (Handle)(*project)->dirs[i].alias;
		if (h != NULL) {
			DisposeHandle(h);
		}
	}
	DisposeHandle((Handle)project);
}

// ProjectCreateWindow creates a window for a given Project object.
static void ProjectCreateWindow(ProjectHandle project)
{
	WindowRef window;
	ControlRef control;
	int i, windowWidth, controlWidth;

	window = GetNewCWindow(rWIND_Project, NULL, (WindowPtr)-1);
	if (window == NULL) {
		EXIT_INTERNAL();
	}
	windowWidth = window->portRect.right - window->portRect.left;
	SetWRefCon(window, (long)project);
	(*project)->windowWidth = windowWidth;

	for (i = 0; i < 2; i++) {
		control = GetNewControl(rCNTL_ChooseFolder, window);
		if (control == NULL) {
			EXIT_INTERNAL();
		}
		controlWidth =
			(*control)->contrlRect.right - (*control)->contrlRect.left;
		MoveControl(control, windowWidth - controlWidth - kHBorder,
		            kVBorder + kDirVSize * i + 22);
		SetControlReference(control, kControlChooseDirLocal + i);
	}

	ShowWindow(window);
}

void ProjectNew(void)
{
	ProjectHandle project;

	project = ProjectNewObject();
	if (project == NULL) {
		return;
	}
	ProjectCreateWindow(project);
}

// ProjectReadError displays an error message encountered when trying to read a
// project.
static void ProjectReadError(ProjectHandle project, ErrorCode err, OSErr osErr)
{
	StrFileName name;

	memcpy(name, (*project)->fileSpec.name, sizeof(StrFileName));
	ShowError(kErrCouldNotReadProject, err, osErr, name);
}

typedef enum ProjectChunkStatus {
	kChunkNotFound,
	kChunkFound,
	kChunkError,
} ProjectChunkStatus;

static ProjectChunkStatus ProjectReadChunk(ProjectHandle project, int nchunks,
                                           struct ProjectChunk **chunks,
                                           UInt32 chunkID, Handle *dataPtr,
                                           long *sizePtr)
{
	struct ProjectChunk *chunk, *end;
	UInt32 offset, size, crc32, gotCRC32;
	long count;
	Handle h;
	OSErr osErr;
	short fileRef;

	chunk = *chunks;
	end = chunk + nchunks;
	for (; chunk < end; chunk++) {
		if (chunk->id == chunkID) {
			goto found;
		}
	}
	return kChunkNotFound;

found:
	offset = chunk->offset;
	size = chunk->size;
	crc32 = chunk->crc32;
	if (size > kMaxChunkSize) {
		ProjectReadError(project, kErrProjectLarge, 0);
		return kChunkError;
	}
	h = NewHandle(size);
	if (h == NULL) {
		ProjectReadError(project, kErrOutOfMemory, MemError());
		return kChunkError;
	}
	if (size > 0) {
		fileRef = (*project)->fileRef;
		osErr = SetFPos(fileRef, fsFromStart, offset);
		if (osErr != 0) {
			DisposeHandle(h);
			ProjectReadError(project, kErrNone, osErr);
			return kChunkError;
		}
		count = size;
		osErr = FSRead(fileRef, &count, *h);
		if (osErr != 0) {
			DisposeHandle(h);
			ProjectReadError(project, kErrNone, osErr);
			return kChunkError;
		}
	}
	gotCRC32 = CRC32Update(0, *h, size);
	if (crc32 != gotCRC32) {
		DisposeHandle(h);
		ProjectReadError(project, kErrProjectDamaged, 0);
		return kChunkError;
	}
	*dataPtr = h;
	*sizePtr = size;
	return kChunkFound;
}

// ProjectRead reads the project from disk. Returns true on success.
static Boolean ProjectRead(ProjectHandle project)
{
	struct ProjectHeader header;
	struct ProjectChunk **chunks;
	short fileRef;
	long count, size;
	ErrorCode err;
	OSErr osErr;
	int nchunks, i, pathLength;
	UInt32 headerCRC, gotCRC;
	Handle alias, path;
	ProjectChunkStatus chunkStatus;
	FSSpec folderSpec;
	Boolean wasChanged;

	chunks = NULL;
	err = kErrNone;
	osErr = 0;

	fileRef = (*project)->fileRef;
	osErr = SetFPos(fileRef, fsFromStart, 0);
	if (osErr != 0) {
		goto error;
	}
	count = sizeof(header);
	osErr = FSRead(fileRef, &count, &header);
	if (osErr != 0) {
		goto error;
	}
	if (count < sizeof(header) ||
	    memcmp(header.magic, kMagic, sizeof(kMagic)) != 0) {
		err = kErrProjectDamaged;
		goto error;
	}
	if (header.chunkCount > kMaxChunkCount) {
		err = kErrProjectLarge;
		goto error;
	}
	headerCRC = header.crc32;
	header.crc32 = 0;
	gotCRC = CRC32Update(0, &header, sizeof(header));
	nchunks = header.chunkCount;
	if (nchunks > 0) {
		size = sizeof(struct ProjectChunk) * nchunks;
		chunks = (struct ProjectChunk **)NewHandle(size);
		if (chunks == NULL) {
			err = kErrOutOfMemory;
			osErr = MemError();
			goto error;
		}
		count = size;
		osErr = FSRead(fileRef, &count, *chunks);
		if (osErr != 0) {
			goto error;
		}
		if (count < size) {
			err = kErrProjectDamaged;
			goto error;
		}
		gotCRC = CRC32Update(gotCRC, *chunks, size);
	}
	if (headerCRC != gotCRC) {
		err = kErrProjectDamaged;
		goto error;
	}

	for (i = 0; i < 2; i++) {
		chunkStatus = ProjectReadChunk(project, nchunks, chunks,
		                               kProjectAliasChunks[i], &alias, &size);
		switch (chunkStatus) {
		case kChunkError:
			goto errorSilent;
		case kChunkFound:
			(*project)->dirs[i].alias = (AliasHandle)alias;
			osErr = ResolveAlias(NULL, (AliasHandle)alias, &folderSpec,
			                     &wasChanged);
			err = (ErrorCode)(kErrLocalNotFound + i);
			switch (osErr) {
			case noErr:
				osErr = GetDirPath(&folderSpec, &pathLength, &path);
				if (osErr != 0) {
					ShowError(err, kErrNone, osErr, NULL);
				} else {
					(*project)->dirs[i].pathLength = pathLength;
					(*project)->dirs[i].path = path;
				}
				break;
			case nsvErr:
				ShowError(err, kErrNotMounted, 0, NULL);
				break;
			case fnfErr:
			case dirNFErr:
				ShowError(err, kErrNone, 0, NULL);
				break;
			default:
				ShowError(err, kErrNone, osErr, NULL);
				break;
			}
			break;
		default:
			chunkStatus = ProjectReadChunk(project, nchunks, chunks,
			                               kProjectPathChunks[i], &path, &size);
			switch (chunkStatus) {
			case kChunkError:
				goto errorSilent;
			case kChunkFound:
				(*project)->dirs[i].pathLength = size;
				(*project)->dirs[i].path = path;
				break;
			default:
				break;
			}
			break;
		}
	}

	if (chunks != NULL) {
		DisposeHandle((Handle)chunks);
	}
	return true;

error:
	if (chunks != NULL) {
		DisposeHandle((Handle)chunks);
	}
	ProjectReadError(project, err, osErr);
	return false;

errorSilent:
	if (chunks != NULL) {
		DisposeHandle((Handle)chunks);
	}
	return false;
}

void ProjectOpen(FSSpec *spec, ScriptCode script)
{
	ProjectHandle project;
	struct Project *projectp;
	short fileRef;
	OSErr err;

	err = FSpOpenDF(spec, fsRdWrPerm, &fileRef);
	if (err != noErr) {
		ShowError(kErrCouldNotReadProject, kErrNone, err, spec->name);
		return;
	}
	project = ProjectNewObject();
	if (project == NULL) {
		FSClose(fileRef);
		return;
	}
	projectp = *project;
	projectp->fileRef = fileRef;
	memcpy(&projectp->fileSpec, spec, sizeof(FSSpec));
	projectp->fileScript = script;

	if (!ProjectRead(project)) {
		// This will close the file.
		ProjectDelete(project);
		return;
	}

	ProjectCreateWindow(project);
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
	KillControls(window);
	DisposeWindow(window);
	ProjectDelete(project);
}

#define kChunkCount 4

#define ALIGN(x) (((x) + 15) & ~(UInt32)15)

static OSErr ProjectWriteHeader(short refNum, int nchunks,
                                struct ProjectChunk *ckInfo)
{
	Ptr ptr;
	struct ProjectHeader *head;
	UInt32 size;
	long count;
	OSErr err;

	size = sizeof(struct ProjectHeader) + sizeof(struct ProjectChunk) * nchunks;
	ptr = NewPtr(size);
	if (ptr == NULL) {
		return MemError();
	}
	head = (struct ProjectHeader *)ptr;
	memcpy(head->magic, kMagic, sizeof(head->magic));
	head->version = kVersion;
	head->size = size;
	head->crc32 = 0;
	head->chunkCount = nchunks;
	memcpy(ptr + sizeof(struct ProjectHeader), ckInfo,
	       nchunks * sizeof(struct ProjectChunk));
	head->crc32 = CRC32Update(0, ptr, size);

	count = size;
	err = FSWrite(refNum, &count, ptr);
	DisposePtr(ptr);
	return err;
}

static OSErr ProjectFlush(short refNum)
{
	ParamBlockRec pb;

	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioRefNum = refNum;
	return PBFlushFileSync(&pb);
}

// ProjectWriteContent writes the project data to the given file.
static OSErr ProjectWriteContent(ProjectHandle project, short refNum)
{
	UInt32 pad[4];
	OSErr err;
	Handle ckData[kChunkCount];
	struct ProjectChunk ckInfo[kChunkCount];
	UInt32 off;
	Size size;
	Handle h;
	int i, nchunks;
	long count;

	// Gather all chunks.
	nchunks = 0;
	for (i = 0; i < 2; i++) {
		h = (Handle)(*project)->dirs[i].alias;
		if (h != NULL) {
			ckData[nchunks] = h;
			ckInfo[nchunks].id = kProjectAliasChunks[i];
			nchunks++;
		}
		h = (*project)->dirs[i].path;
		if (h != NULL) {
			ckData[nchunks] = h;
			ckInfo[nchunks].id = kProjectPathChunks[i];
			nchunks++;
		}
	}

	// Calculate chunk offsets.
	off = sizeof(struct ProjectHeader) + sizeof(struct ProjectChunk) * nchunks;
	for (i = 0; i < nchunks; i++) {
		size = GetHandleSize(ckData[i]);
		ckInfo[i].offset = off;
		ckInfo[i].size = size;
		ckInfo[i].crc32 = CRC32Update(0, *ckData[i], size);
		off = ALIGN(off + size);
	}

	// Write file. Refer to IM: Files listing 1-9, page 1-24.
	err = SetEOF(refNum, off);
	if (err != noErr) {
		return err;
	}
	err = ProjectWriteHeader(refNum, nchunks, ckInfo);
	if (err != noErr) {
		return err;
	}
	for (i = 0; i < 4; i++) {
		pad[i] = 0;
	}
	for (i = 0; i < nchunks; i++) {
		size = ckInfo[i].size;
		count = size;
		err = FSWrite(refNum, &count, *ckData[i]);
		if (err != noErr) {
			return err;
		}
		count = (-size) & 15;
		if (count > 0) {
			err = FSWrite(refNum, &count, pad);
			if (err != noErr) {
				return err;
			}
		}
	}

	return noErr;
}

// A SaveMode describes the exact type of save operation.
typedef enum {
	// kSaveNew indicates that the file is new, it does not exist yet.
	kSaveNew,
	// kSaveReplace indicates that the file is being saved over an existing
	// file.
	kSaveReplace,
	// kSaveUpdate indicates that the file is being updated in-place.
	kSaveUpdate
} SaveMode;

// ProjectCloseFile closes any open files and sets the file ref to 0.
static void ProjectCloseFile(ProjectHandle project)
{
	struct Project *projectp;

	projectp = *project;
	if (projectp->fileRef != 0) {
		FSClose(projectp->fileRef);
	}
}

// ProjectSetFile closes any open files and replaces them with the given file.
static void ProjectSetFile(ProjectHandle project, short fileRef, FSSpec *spec)
{
	struct Project *projectp;

	projectp = *project;
	if (projectp->fileRef != 0) {
		FSClose(projectp->fileRef);
	}
	projectp->fileRef = fileRef;
	memcpy(&projectp->fileSpec, spec, sizeof(FSSpec));
}

// ProjectWrite writes the project to a file.
static OSErr ProjectWrite(ProjectHandle project, FSSpec *spec,
                          ScriptCode script, SaveMode mode)
{
	HParamBlockRec hb;
	CMovePBRec cm;
	FSSpec temp;
	OSErr err;
	short refNum;

	// See IM: Files, p. 1-25, Listing 1-10 "Updating a File Safely".

	err = MakeTempFile(spec->vRefNum, &temp);
	if (err != noErr) {
		return err;
	}
	err = FSpCreate(&temp, kCreator, kTypeProject, script);
	if (err != noErr) {
		return err;
	}
	err = FSpOpenDF(&temp, fsRdWrPerm, &refNum);
	if (err != noErr) {
		goto error1;
	}
	err = ProjectWriteContent(project, refNum);
	if (err != noErr) {
		goto error2;
	}

	// For ordinary "Save", swap the file contents.
	if (mode == kSaveUpdate) {
		err = FSpExchangeFiles(&temp, spec);
		if (err == noErr) {
			err = FlushVol(NULL, spec->vRefNum);
			if (err != noErr) {
				FSClose(refNum);
				return err;
			}
			ProjectSetFile(project, refNum, spec);
			// FIXME: What error handling, here?
			FSpDelete(&temp);
			return noErr;
		}
		// paramErr: function not supported by volume.
		if (err != paramErr) {
			goto error2;
		}
	}

	ProjectCloseFile(project);
	if (mode != kSaveNew) {
		// For replace or update, the file already exists.
		err = FSpDelete(spec);
		if (err != noErr && err != fnfErr) {
			goto error2;
		}
	}

	// Try move & rename.
	memset(&hb, 0, sizeof(hb));
	hb.copyParam.ioNamePtr = temp.name;
	hb.copyParam.ioVRefNum = temp.vRefNum;
	hb.copyParam.ioNewName = spec->name;
	hb.copyParam.ioNewDirID = spec->parID;
	hb.copyParam.ioDirID = temp.parID;
	err = PBHMoveRenameSync(&hb);
	if (err == noErr) {
		goto done;
	}
	// paramErr: function not supported by volume.
	if (err != paramErr) {
		goto error2;
	}

	// Try move, then rename.
	memset(&cm, 0, sizeof(cm));
	cm.ioNamePtr = temp.name;
	cm.ioVRefNum = temp.vRefNum;
	cm.ioNewDirID = spec->parID;
	cm.ioDirID = temp.parID;
	err = PBCatMoveSync(&cm);
	if (err != noErr) {
		goto error2;
	}
	temp.parID = spec->parID;
	err = FSpRename(&temp, spec->name);
	if (err != noErr) {
		goto error2;
	}
done:
	err = FlushVol(NULL, spec->vRefNum);
	if (err != noErr) {
		FSClose(refNum);
		return err;
	}
	ProjectSetFile(project, refNum, spec);
	return noErr;

error2:
	FSClose(refNum);
error1:
	FSpDelete(&temp);
	return err;
}

// A SaveCommand distinguishes between Save and Save As.
typedef enum { kSave, kSaveAs } SaveCommand;

// ProjectSave handles the Save and SaveAs menu commands.
static void ProjectSave(WindowRef window, ProjectHandle project,
                        SaveCommand cmd)
{
	// See listing 1-12 in IM: Files.
	Str255 name;
	StandardFileReply reply;
	OSErr err;
	FSSpec spec;
	ScriptCode script;

	if (cmd == kSaveAs || (*project)->fileSpec.vRefNum == 0) {
		GetWTitle(window, name);
		// FIXME: Move string to resource.
		StandardPutFile("\pSave project as:", name, &reply);
		if (!reply.sfGood) {
			return;
		}

		err = ProjectWrite(project, &reply.sfFile, reply.sfScript,
		                   reply.sfReplacing ? kSaveReplace : kSaveNew);
		if (err != noErr) {
			goto error;
		}
		SetWTitle(window, reply.sfFile.name);
	} else {
		memcpy(&spec, &(*project)->fileSpec, sizeof(FSSpec));
		script = (*project)->fileScript;
		err = ProjectWrite(project, &spec, script, kSaveUpdate);
		if (err != noErr) {
			goto error;
		}
	}

	return;

error:
	ShowError(kErrCouldNotSaveProject, kErrNone, err, reply.sfFile.name);
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
			ProjectSave(window, project, kSave);
			break;
		case iFile_SaveAs:
			ProjectSave(window, project, kSaveAs);
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

// VolumeDoesSupportIDs returns true if the volume supports file IDs. If the
// volume does not support file IDs, then we fall back to using paths to refer
// to project directories. This applies to the Basilisk II "Unix" volume. If you
// try to resolve an alias to a file on the Basilisk II "Unix" volume, you may
// get a different file than intended, because the directory IDs are not stable.
static Boolean VolumeDoesSupportIDs(short vRefNum)
{
	HParamBlockRec hb;
	GetVolParmsInfoBuffer parms;
	OSErr err;

	memset(&hb, 0, sizeof(hb));
	hb.ioParam.ioVRefNum = vRefNum;
	hb.ioParam.ioBuffer = (Ptr)&parms;
	hb.ioParam.ioReqCount = sizeof(parms);
	err = PBHGetVolParms(&hb, false);
	if (err != noErr) {
		ShowError(kErrVolumeQuery, kErrNone, err, NULL);
		return false;
	}
	return (parms.vMAttrib & (1ul << bHasFileIDs)) != 0;
}

static void ProjectChooseDir(WindowRef window, ProjectHandle project,
                             int whichDir)
{
	struct Project *projectp;
	struct ProjectDir *dirp;
	FSSpec directory;
	AliasHandle alias;
	Handle path;
	int pathLength;
	OSErr err;

	if (!ChooseDirectory(&directory)) {
		return;
	}
	if (VolumeDoesSupportIDs(directory.vRefNum)) {
		err = NewAlias(NULL, &directory, &alias);
		if (err != noErr) {
			ShowError(kErrAlias, kErrNone, err, NULL);
			return;
		}
	} else {
		alias = NULL;
	}
	err = GetDirPath(&directory, &pathLength, &path);
	if (err != noErr) {
		ShowError(kErrDirPath, kErrNone, err, NULL);
		if (alias != NULL) {
			DisposeHandle((Handle)alias);
		}
		return;
	}

	projectp = *project;
	dirp = &projectp->dirs[whichDir];
	if (dirp->path != NULL) {
		DisposeHandle(dirp->path);
	}
	dirp->pathLength = pathLength;
	dirp->path = path;
	if (dirp->alias != NULL) {
		DisposeHandle((Handle)dirp->alias);
	}
	dirp->alias = alias;
	InvalRect(&window->portRect);
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

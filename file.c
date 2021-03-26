#include "defs.h"

#include "convert.h"

#include <Files.h>
#include <Script.h>

#include <string.h>

// Make an FSSpec for a temporary file.
static int make_temp(FSSpec *temp, short vRefNum, long dirID,
                     const unsigned char *name) {
	Str31 tname;
	unsigned pfxlen, maxpfx = 31 - 4;
	OSErr err;

	pfxlen = name[0];
	if (pfxlen > maxpfx) {
		pfxlen = maxpfx;
	}
	memcpy(tname + 1, name + 1, pfxlen);
	memcpy(tname + 1 + pfxlen, ".tmp", 4);
	tname[0] = pfxlen + 4;
	err = FSMakeFSSpec(vRefNum, dirID, tname, temp);
	if (err != 0 && err != fnfErr) {
		print_errcode(err, "could not create temp file spec");
		return 1;
	}
	return 0;
}

// Set the modification time for a file.
static int set_modtime(FSSpec *spec, long modTime) {
	CInfoPBRec ci;
	Str31 name;
	OSErr err;

	memset(&ci, 0, sizeof(ci));
	memcpy(name, spec->name, spec->name[0] + 1);
	ci.hFileInfo.ioNamePtr = name;
	ci.hFileInfo.ioVRefNum = spec->vRefNum;
	ci.hFileInfo.ioDirID = spec->parID;
	err = PBGetCatInfoSync(&ci);
	if (err != 0) {
		print_errcode(err, "could not get temp file info");
		return 1;
	}
	memcpy(name, spec->name, spec->name[0] + 1);
	ci.hFileInfo.ioNamePtr = name;
	ci.hFileInfo.ioVRefNum = spec->vRefNum;
	ci.hFileInfo.ioDirID = spec->parID;
	ci.hFileInfo.ioFlMdDat = modTime;
	err = PBSetCatInfoSync(&ci);
	if (err != 0) {
		print_errcode(err, "could not set temp file info");
		return 1;
	}
	return 0;
}

// Move a temp file over a destination file. This may modify the temp file spec
// if it moves in multiple stages.
static int replace_file(FSSpec *temp, FSSpec *dest, file_action action) {
	HParamBlockRec pb;
	CMovePBRec cm;
	OSErr err;
	bool mustMove, mustRename;

	// First, try to exchange files if destination exists.
	if (action == kActionReplace) {
		err = FSpExchangeFiles(temp, dest);
		if (gLogLevel >= kLogVerbose) {
			log_call(err, "FSpExchangeFiles");
		}
		if (err == 0) {
			err = FSpDelete(temp);
			if (err != 0) {
				print_errcode(err, "could not remove temporary file");
				return 1;
			}
			return 0;
		}
		// paramErr: function not supported by volume.
		if (err != paramErr) {
			print_errcode(err, "could not exchange files");
			return 1;
		}
		// Otherwise, delete destination and move temp file over.
		err = FSpDelete(dest);
		if (err != 0) {
			print_errcode(err, "could not remove destination file");
			return 1;
		}
	}

	mustMove = dest->parID != temp->parID;
	mustRename = memcmp(dest->name, temp->name, dest->name[0] + 1) != 0;

	// Next, try MoveRename.
	if (mustMove && mustRename) {
		memset(&pb, 0, sizeof(pb));
		pb.copyParam.ioNamePtr = temp->name;
		pb.copyParam.ioVRefNum = temp->vRefNum;
		pb.copyParam.ioNewName = dest->name;
		pb.copyParam.ioNewDirID = dest->parID;
		pb.copyParam.ioDirID = temp->parID;
		err = PBHMoveRenameSync(&pb);
		if (gLogLevel >= kLogVerbose) {
			log_call(err, "PBHMoveRename");
		}
		if (err == 0) {
			return 0;
		}
		// paramErr: function not supported by volume.
		if (err != paramErr) {
			print_errcode(err, "could not rename temporary file");
			return 1;
		}
	}

	// Finally, try move and then rename.
	if (mustMove) {
		memset(&cm, 0, sizeof(cm));
		cm.ioNamePtr = temp->name;
		cm.ioVRefNum = temp->vRefNum;
		cm.ioNewDirID = dest->parID;
		cm.ioDirID = temp->parID;
		err = PBCatMoveSync(&cm);
		if (gLogLevel >= kLogVerbose) {
			log_call(err, "PBCatMove");
		}
		if (err != 0) {
			print_errcode(err, "could not move temporary file");
			return 1;
		}
		temp->parID = dest->parID;
	}
	if (mustRename) {
		err = FSpRename(temp, dest->name);
		if (gLogLevel >= kLogVerbose) {
			log_call(err, "FSpRename");
		}
		if (err != 0) {
			print_errcode(err, "could not rename temporary file");
			return 1;
		}
	}

	return 0;
}

static Ptr gSrcBuffer;
static Ptr gDestBuffer;

int sync_file(struct file_info *file, operation_mode mode, short srcVol,
              long srcDir, short destVol, long destDir, short tempVol,
              long tempDir) {
	OSType creator, fileType;
	FSSpec src, dest, temp;
	short srcRef = 0, destRef = 0;
	bool has_temp = false;
	int r;
	OSErr err;

	// Handle actions which don't involve conversion.
	if (file->action == kActionNone) {
		return 0;
	}
	if (file->action == kActionDelete) {
		err = FSMakeFSSpec(destVol, destDir, file->name, &dest);
		if (err != 0) {
			print_errcode(err, "could not create destination spec");
			return 1;
		}
		err = FSpDelete(&dest);
		if (err != 0) {
			print_errcode(err, "could not delete destination file");
			return 1;
		}
		return 0;
	}

	// Create file specs.
	err = FSMakeFSSpec(srcVol, srcDir, file->name, &src);
	if (err != 0) {
		print_errcode(err, "could not create source spec");
		return 1;
	}
	err = FSMakeFSSpec(destVol, destDir, file->name, &dest);
	if (err != 0 && err != fnfErr) {
		print_errcode(err, "could not create destination spec");
		return 1;
	}
	r = make_temp(&temp, tempVol, tempDir, dest.name);
	if (r != 0) {
		return 1;
	}

	// Create the temporary file.
	switch (file->type) {
	case kTypeText:
		creator = 'MPS ';
		fileType = 'TEXT';
		break;
	case kTypeResource:
		creator = 'RSED';
		fileType = 'rsrc';
		break;
	default:
		print_err("invalid type");
		return 1;
	}
	err = FSpCreate(&temp, creator, fileType, smSystemScript);
	if (err == dupFNErr) {
		err = FSpDelete(&temp);
		if (err != 0) {
			print_errcode(err, "could not delete existing temp file");
			goto error;
		}
		err = FSpCreate(&temp, creator, fileType, smSystemScript);
	}
	if (err != 0) {
		print_errcode(err, "could not create file");
		goto error;
	}
	has_temp = true;

	// Get buffers for conversion.
	if (gSrcBuffer == NULL) {
		gSrcBuffer = NewPtr(kBufferTotalSize);
		if (gSrcBuffer == NULL) {
			print_memerr(kBufferTotalSize);
			goto error;
		}
	}
	if (gDestBuffer == NULL) {
		gDestBuffer = NewPtr(kBufferTotalSize);
		if (gDestBuffer == NULL) {
			print_memerr(kBufferTotalSize);
			goto error;
		}
	}

	// Open the source file for reading.
	if (file->type == kTypeResource && mode == kModePush) {
		err = FSpOpenRF(&src, fsRdPerm, &srcRef);
	} else {
		err = FSpOpenDF(&src, fsRdPerm, &srcRef);
	}
	if (err != 0) {
		print_errcode(err, "could not open file");
		goto error;
	}
	if (file->type == kTypeResource && mode == kModePull) {
		err = FSpOpenRF(&temp, fsRdWrPerm, &destRef);
	} else {
		err = FSpOpenDF(&temp, fsRdWrPerm, &destRef);
	}
	if (err != 0) {
		print_errcode(err, "could not open temp file");
		goto error;
	}

	// Convert data.
	switch (file->type) {
	case kTypeText:
		if (mode == kModePush) {
			r = mac_to_unix(srcRef, destRef, gSrcBuffer, gDestBuffer);
		} else {
			r = mac_from_unix(srcRef, destRef, gSrcBuffer, gDestBuffer);
		}
		break;
	case kTypeResource:
		r = copy_data(srcRef, destRef, gSrcBuffer);
		break;
	default:
		print_err("invalid type");
		goto error;
	}
	if (r != 0) {
		goto error;
	}

	// Close files.
	err = FSClose(srcRef);
	srcRef = 0;
	if (err != 0) {
		print_errcode(err, "could not close source file");
		goto error;
	}
	err = FSClose(destRef);
	destRef = 0;
	if (err != 0) {
		print_errcode(err, "could not close temp file");
		goto error;
	}

	// Set modification time.
	r = set_modtime(&temp, file->meta[kSrcDir].modTime);
	if (r != 0) {
		goto error;
	}

	// Overwrite destination.
	r = replace_file(&temp, &dest, file->action);
	if (r != 0) {
		goto error;
	}
	return 0;

error:
	// Clean up.
	if (srcRef != 0) {
		err = FSClose(srcRef);
		if (err != 0) {
			print_errcode(err, "could not close source file");
		}
	}
	if (destRef != 0) {
		err = FSClose(destRef);
		if (err != 0) {
			print_errcode(err, "could not close destination file");
		}
	}
	if (has_temp) {
		err = FSpDelete(&temp);
		if (err != 0) {
			print_errcode(err, "could not delete temp file");
		}
	}
	return 1;
}

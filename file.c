#include "defs.h"

#include <Files.h>
#include <Script.h>

#include <string.h>

enum {
	// Maximum file size that we will copy.
	kMaxFileSize = 64 * 1024,
};

// Read the entire data fork of a file. The result must be freed with
// DisposePtr.
static int read_file(FSSpec *spec, Ptr *data, long *length) {
	CInfoPBRec ci;
	Ptr ptr;
	long dataLength, pos, count;
	OSErr err;
	short fref;

	// Get file size.
	memset(&ci, 0, sizeof(ci));
	ci.hFileInfo.ioNamePtr = spec->name;
	ci.hFileInfo.ioVRefNum = spec->vRefNum;
	ci.hFileInfo.ioDirID = spec->parID;
	err = PBGetCatInfoSync(&ci);
	if (err != 0) {
		print_errcode(err, "could not get file metadata");
		return 1;
	}
	if ((ci.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0) {
		print_err("is a directory");
		return 1;
	}
	dataLength = ci.hFileInfo.ioFlLgLen;
	if (dataLength > kMaxFileSize) {
		print_err("file is too large: size=%ld, max=%ld", dataLength,
		          kMaxFileSize);
		return 1;
	}
	if (dataLength == 0) {
		*data = NULL;
		*length = 0;
		return 0;
	}

	// Allocate memory.
	ptr = NewPtr(dataLength);
	if (ptr == NULL) {
		print_memerr(dataLength);
		return 1;
	}

	// Read file.
	err = FSpOpenDF(spec, fsRdPerm, &fref);
	if (err != 0) {
		DisposePtr(ptr);
		print_errcode(err, "could not open file");
		return 1;
	}
	pos = 0;
	while (pos < dataLength) {
		count = dataLength - pos;
		err = FSRead(fref, &count, ptr + pos);
		if (err != 0) {
			DisposePtr(ptr);
			FSClose(fref);
			print_errcode(err, "could not read file");
			return 1;
		}
		pos += count;
	}
	FSClose(fref);
	*data = ptr;
	*length = dataLength;
	return 0;
}

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

// Write the entire contents of a file.
static int write_file(FSSpec *dest, short tempVol, long tempDir, Ptr data,
                      long length, long modTime, file_action action) {
	OSType creator = 'MPS ', fileType = 'TEXT';
	FSSpec temp;
	long pos, amt;
	short ref;
	HParamBlockRec pb;
	CMovePBRec cm;
	CInfoPBRec ci;
	Str31 name;
	OSErr err;
	int r;

	// Save the data to a temporary file.
	r = make_temp(&temp, tempVol, tempDir, dest->name);
	if (r != 0) {
		return 1;
	}
	err = FSpCreate(&temp, creator, fileType, smSystemScript);
	if (err == dupFNErr) {
		err = FSpDelete(&temp);
		if (err != 0) {
			print_errcode(err, "could not delete existing temp file");
			return 1;
		}
		err = FSpCreate(&temp, creator, fileType, smSystemScript);
	}
	if (err != 0) {
		print_errcode(err, "could not create file");
		return 1;
	}
	err = FSpOpenDF(&temp, fsRdWrPerm, &ref);
	if (err != 0) {
		print_errcode(err, "could not open temp file");
		goto error;
	}
	pos = 0;
	while (pos < length) {
		amt = length - pos;
		err = FSWrite(ref, &amt, data + pos);
		if (err != 0) {
			FSClose(ref);
			print_errcode(err, "could not write temp file");
			goto error;
		}
		pos += amt;
	}
	err = FSClose(ref);
	if (err != 0) {
		print_errcode(err, "could not close temp file");
		goto error;
	}

	// Update the modification time.
	memset(&ci, 0, sizeof(ci));
	memcpy(name, temp.name, temp.name[0] + 1);
	ci.hFileInfo.ioNamePtr = name;
	ci.hFileInfo.ioVRefNum = temp.vRefNum;
	ci.hFileInfo.ioDirID = temp.parID;
	err = PBGetCatInfoSync(&ci);
	if (err != 0) {
		print_errcode(err, "could not get temp file info");
		goto error;
	}
	memcpy(name, temp.name, temp.name[0] + 1);
	ci.hFileInfo.ioNamePtr = name;
	ci.hFileInfo.ioVRefNum = temp.vRefNum;
	ci.hFileInfo.ioDirID = temp.parID;
	ci.hFileInfo.ioFlMdDat = modTime;
	err = PBSetCatInfoSync(&ci);
	if (err != 0) {
		print_errcode(err, "could not set temp file info");
		goto error;
	}

	// First, try to exchange files if destination exists.
	if (action == kActionReplace) {
		err = FSpExchangeFiles(&temp, dest);
		if (gLogLevel >= kLogVerbose) {
			log_call(err, "FSpExchangeFiles");
		}
		if (err == 0) {
			err = FSpDelete(&temp);
			if (err != 0) {
				print_errcode(err, "could not remove temporary file");
				return 1;
			}
			return 0;
		}
		// paramErr: function not supported by volume.
		if (err != paramErr) {
			print_errcode(err, "could not exchange files");
			goto error;
		}
		// Otherwise, delete destination and move temp file over.
		err = FSpDelete(dest);
		if (err != 0) {
			print_errcode(err, "could not remove destination file");
			goto error;
		}
	}

	// Next, try MoveRename.
	memset(&pb, 0, sizeof(pb));
	pb.copyParam.ioNamePtr = temp.name;
	pb.copyParam.ioVRefNum = temp.vRefNum;
	pb.copyParam.ioNewName = dest->name;
	pb.copyParam.ioNewDirID = dest->parID;
	pb.copyParam.ioDirID = temp.parID;
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
		goto error;
	}

	// Finally, try move and then rename.
	if (dest->parID != temp.parID) {
		memset(&cm, 0, sizeof(cm));
		cm.ioNamePtr = temp.name;
		cm.ioVRefNum = temp.vRefNum;
		cm.ioNewDirID = dest->parID;
		cm.ioDirID = temp.parID;
		err = PBCatMoveSync(&cm);
		if (gLogLevel >= kLogVerbose) {
			log_call(err, "PBCatMove");
		}
		if (err != 0) {
			print_errcode(err, "could not move temporary file");
			goto error;
		}
		temp.parID = dest->parID;
	}
	if (memcmp(dest->name, temp.name, dest->name[0] + 1) != 0) {
		err = FSpRename(&temp, dest->name);
		if (gLogLevel >= kLogVerbose) {
			log_call(err, "FSpRename");
		}
		if (err != 0) {
			print_errcode(err, "could not rename temporary file");
			goto error;
		}
	}

	return 0;

error:
	err = FSpDelete(&temp);
	if (err != 0) {
		print_errcode(err, "could not delete temp file");
	}
	return 1;
}

int sync_file(struct file_info *file, convert_func func, long srcVol,
              short srcDir, long destVol, short destDir, long tempVol,
              short tempDir) {
	FSSpec src, dest;
	Ptr srcData = NULL, destData = NULL;
	long srcLength, destLength;
	int r, result = 1;
	OSErr err;
	unsigned char *outptr, *outend;
	const unsigned char *inptr, *inend;

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

	// Read the source file into memory.
	r = read_file(&src, &srcData, &srcLength);
	if (r != 0) {
		return 1;
	}

	// Convert data.
	if (srcLength > 0) {
		destLength = srcLength + (srcLength >> 2) + 16;
		destData = NewPtr(destLength);
		if (destData == NULL) {
			print_memerr(destLength);
			goto done;
		}
		outptr = (unsigned char *)destData;
		outend = outptr + destLength;
		inptr = (unsigned char *)srcData;
		inend = inptr + srcLength;
		func(&outptr, outend, &inptr, inend);
		if (inptr != inend) {
			print_err("conversion function failed");
			goto done;
		}
		destLength = outptr - (unsigned char *)destData;
	} else {
		destLength = 0;
		destData = NULL;
	}

	// Write destination file.
	r = write_file(&dest, tempVol, tempDir, destData, destLength,
	               file->meta[kSrcDir].modTime, file->action);
	if (r != 0) {
		goto done;
	}

	// Success.
	result = 0;

done:
	// Clean up.
	if (srcData != NULL) {
		DisposePtr(srcData);
	}
	if (destData != NULL) {
		DisposePtr(destData);
	}
	return result;
}

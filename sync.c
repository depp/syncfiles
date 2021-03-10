#include "defs.h"

#include <Files.h>
#include <Folders.h>
#include <MacMemory.h>
#include <StringCompare.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

enum {
	// Maximum file size that we will copy.
	kMaxFileSize = 64 * 1024,
};

typedef void (*convert_func)(unsigned char **outptr, unsigned char *outend,
                             const unsigned char **inptr,
                             const unsigned char *inend);

static void print_err(const char *msg, ...) {
	va_list ap;
	fputs("## Error: ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);
}

// Map from OSErr codes to messages. As a heuristic, this should include error
// codes caused by toolbox calls in this program which are readily understood by
// the user. It can exclude error codes that are likely to indicate a
// programming error.
struct error_message {
	OSErr err;
	const char *msg;
};
const struct error_message kErrorMessages[] = {
	{dirFulErr, "directory full"},                                   // -33
	{dskFulErr, "disk full"},                                        // -34
	{ioErr, "I/O error"},                                            // -36
	{bdNamErr, "bad name"},                                          // -37
	{fnfErr, "file not found"},                                      // -43
	{wPrErr, "disk is write-protected"},                             // -44
	{fLckdErr, "file is locked"},                                    // -45
	{vLckdErr, "volume is locked"},                                  // -46
	{dupFNErr, "destination already exists"},                        // -48
	{opWrErr, "file already open for writing"},                      // -49
	{paramErr, "parameter error"},                                   // -50
	{permErr, "cannot write to locked file"},                        // -54
	{dirNFErr, "directory not found"},                               // -120
	{wrgVolTypErr, "not an HFS volume"},                             // -123
	{diffVolErr, "files on different volumes"},                      // -1303
	{afpAccessDenied, "user does not have access privileges (AFP)"}, // -5000
	{afpObjectTypeErr,
     "file/directory specified where directory/file expected"}, // -5025
	{afpSameObjectErr, "objects are the same"},                 // -5038
};

static const char *mac_strerror(OSErr err) {
	int i, n = sizeof(kErrorMessages) / sizeof(*kErrorMessages);
	for (i = 0; i < n; i++) {
		if (kErrorMessages[i].err == err) {
			return kErrorMessages[i].msg;
		}
	}
	return NULL;
}

static void print_errcode(OSErr err, const char *msg, ...) {
	va_list ap;
	const char *emsg;

	fputs("## Error: ", stderr);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	emsg = mac_strerror(err);
	if (emsg != NULL) {
		fprintf(stderr, ": %s (%d)\n", emsg, err);
	} else {
		fprintf(stderr, ": err=%d\n", err);
	}
}

// Convert a C to Pascal string.
static int c2pstr(Str255 ostr, const char *istr) {
	size_t n = strlen(istr);
	if (n > 255) {
		print_err("path too long: %s", istr);
		return 1;
	}
	ostr[0] = n;
	memcpy(ostr + 1, istr, n);
	memset(ostr + 1 + n, 0, 255 - n);
	return 0;
}

// Convert a Pascal to C string.
static void p2cstr(char *ostr, const unsigned char *istr) {
	unsigned len = istr[0];
	memcpy(ostr, istr + 1, len);
	ostr[len] = '\0';
}

struct file_meta {
	Boolean exists;
	long modTime;
};

enum {
	kSrcDir,
	kDestDir,
};

enum {
	kModeAuto,
	kModePush,
	kModePull,
};

struct file_info {
	Str31 name;
	struct file_meta meta[2];
	int mode;
};

// Array of metadata entries for files.
static Handle gFiles;
static unsigned gFileCount;
static unsigned gFileAlloc;

// Get the metadata entry for a file with the given name.
static struct file_info *get_file(const unsigned char *name) {
	unsigned i, n = gFileCount, len, newAlloc;
	struct file_info *file, *array;
	Handle newFiles;
	OSErr err;

	len = name[0];
	if (len > 31) {
		fputs("## Error: name too long\n", stderr);
		return NULL;
	}
	if (gFiles == NULL) {
		newAlloc = 8;
		newFiles = NewHandle(newAlloc * sizeof(struct file_info));
		err = MemError();
		if (err != noErr) {
			fputs("## Error: out of memory\n", stderr);
			return NULL;
		}
		gFiles = newFiles;
		gFileAlloc = newAlloc;
	} else {
		array = (struct file_info *)*gFiles;
		for (i = 0; i < n; i++) {
			file = &array[i];
			if (memcmp(file->name, name, len + 1) == 0) {
				return file;
			}
		}
		if (n >= gFileAlloc) {
			newAlloc = n * 2;
			SetHandleSize(gFiles, newAlloc * sizeof(struct file_info));
			err = MemError();
			if (err != noErr) {
				fputs("## Error: out of memory\n", stderr);
				return NULL;
			}
			gFileAlloc = newAlloc;
		}
	}
	array = (struct file_info *)*gFiles;
	file = &array[n];
	gFileCount = n + 1;
	memset(file, 0, sizeof(*file));
	memcpy(file->name, name, len + 1);
	return file;
}

// Get the volume and directory ID for a directory from its path.
static int dir_from_path(short *vRefNum, long *dirID, const char *dirpath) {
	Str255 ppath;
	FSSpec spec;
	CInfoPBRec ci;
	OSErr err;

	if (c2pstr(ppath, dirpath)) {
		return 1;
	}
	err = FSMakeFSSpec(0, 0, ppath, &spec);
	if (err != 0) {
		if (err == fnfErr) {
			print_err("does not exist: %s", dirpath);
		} else {
			print_errcode(err, "FSMakeFSSpec");
		}
		return 1;
	}
	memset(&ci, 0, sizeof(ci));
	ci.dirInfo.ioNamePtr = spec.name;
	ci.dirInfo.ioVRefNum = spec.vRefNum;
	ci.dirInfo.ioDrDirID = spec.parID;
	err = PBGetCatInfoSync(&ci);
	if (err != 0) {
		print_errcode(err, "PBGetCatInfoSync");
		return 1;
	}
	if ((ci.dirInfo.ioFlAttrib & kioFlAttribDirMask) == 0) {
		print_err("not a directory: %s", dirpath);
		return 1;
	}
	*vRefNum = ci.dirInfo.ioVRefNum;
	*dirID = ci.dirInfo.ioDrDirID;
	return 0;
}

// Return true if a file with the given name should be included. The name is a
// Pascal string.
static int filter_name(unsigned char *name) {
	int len, i, stem;
	unsigned char *ext;

	if (EqualString(name, "\pmakefile", FALSE, TRUE)) {
		return 1;
	}
	stem = 0;
	len = name[0];
	for (i = 0; i < len; i++) {
		if (name[i + 1] == '.') {
			stem = i;
		}
	}
	if (stem != 0) {
		ext = name + 2 + stem;
		len -= stem + 1;
		switch (len) {
		case 1:
			if (ext[0] == 'c' || ext[0] == 'h' || ext[0] == 'r') {
				return 1;
			}
			break;
		}
	}
	return 0;
}

// List files in a directory, filter them, and add the files matching the filter
// to the file list. The value of 'which' should be kSrcDir or kDestDir.
static int list_dir(short vRefNum, long dirID, int which) {
	Str255 ppath;
	CInfoPBRec ci;
	struct file_info *file;
	OSErr err;
	int i;

	for (i = 1; i < 100; i++) {
		memset(&ci, 0, sizeof(ci));
		ci.dirInfo.ioNamePtr = ppath;
		ci.dirInfo.ioVRefNum = vRefNum;
		ci.dirInfo.ioDrDirID = dirID;
		ci.dirInfo.ioFDirIndex = i;
		err = PBGetCatInfoSync(&ci);
		if (err != 0) {
			if (err == fnfErr) {
				break;
			}
			print_errcode(err, "could not list directory");
			continue;
		}
		if ((ci.hFileInfo.ioFlAttrib & kioFlAttribDirMask) == 0 &&
		    filter_name(ppath)) {
			ppath[ppath[0] + 1] = '\0';
			file = get_file(ppath);
			file->meta[which].exists = TRUE;
			file->meta[which].modTime = ci.hFileInfo.ioFlMdDat;
		}
	}

	return 0;
}

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

	// Allocate memory.
	ptr = NewPtr(dataLength);
	err = MemError();
	if (err != 0) {
		print_errcode(err, "out of memory");
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
	if (err == 0) {
		print_err("temporary file exists");
		return 1;
	} else if (err == fnfErr) {
		return 0;
	} else {
		print_errcode(err, "could not create temp file spec");
		return 1;
	}
}

// Write the entire contents of a file.
static int write_file(FSSpec *dest, short tempVol, long tempDir, Ptr data,
                      long length, long modTime, Boolean destExists) {
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
	err = FSpCreate(&temp, 'MPS ', 'TEXT', smSystemScript);
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
	if (destExists) {
		err = FSpExchangeFiles(&temp, dest);
		if (err == 0) {
			err = FSpDelete(&temp);
			if (err != 0) {
				print_errcode(err, "could not rename temporary file");
				return 1;
			}
			return 0;
		}
		// paramErr: function not supported by volume.
		if (err != paramErr) {
			print_errcode(err, "could not delete temp file");
			return 1;
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
		if (err != 0) {
			print_errcode(err, "could not move temporary file");
			goto error;
		}
		temp.parID = dest->parID;
	}
	if (memcmp(dest->name, temp.name, dest->name[0] + 1) != 0) {
		err = FSpRename(&temp, dest->name);
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

// Copy the source file to the destination file. A temporary file is created in
// the specified temporary directory.
static int sync_file(struct file_info *file, convert_func func, long srcVol,
                     short srcDir, long destVol, short destDir, long tempVol,
                     short tempDir, long modTime) {
	FSSpec src, dest;
	Ptr srcData = NULL, destData = NULL;
	long srcLength, destLength;
	int r;
	OSErr err;
	unsigned char *outptr, *outend;
	const unsigned char *inptr, *inend;
	Boolean destExists;

	// Create file specs.
	err = FSMakeFSSpec(srcVol, srcDir, file->name, &src);
	if (err != 0) {
		print_errcode(err, "could not create source spec");
		return 1;
	}
	err = FSMakeFSSpec(destVol, destDir, file->name, &dest);
	if (err == 0) {
		destExists = TRUE;
	} else if (err == fnfErr) {
		destExists = FALSE;
	} else if (err != 0) {
		print_errcode(err, "could not create destination spec");
		return 1;
	}

	// Read the source file into memory.
	r = read_file(&src, &srcData, &srcLength);
	if (r != 0) {
		return 1;
	}
	// Convert data.
	destLength = srcLength + (srcLength >> 2) + 16;
	destData = NewPtr(destLength);
	err = MemError();
	if (err != 0) {
		print_errcode(err, "out of memory");
		goto error;
	}
	outptr = (unsigned char *)destData;
	outend = outptr + destLength;
	inptr = (unsigned char *)srcData;
	inend = inptr + srcLength;
	func(&outptr, outend, &inptr, inend);
	if (inptr != inend) {
		print_err("conversion function failed");
		goto error;
	}
	destLength = outptr - (unsigned char *)destData;
	r = write_file(&dest, tempVol, tempDir, destData, destLength, destExists,
	               modTime);
	if (r != 0) {
		goto error;
	}

	// Clean up.
	DisposePtr(srcData);
	DisposePtr(destData);
	return 0;

error:
	if (srcData != NULL) {
		DisposePtr(srcData);
	}
	if (destData != NULL) {
		DisposePtr(destData);
	}
	return 1;
}

static int command_main(char *destpath, int mode) {
	short srcVol, destVol, tempVol;
	long srcDir, destDir, tempDir;
	struct file_info *array, *file, *srcNewer, *destNewer;
	OSErr err;
	int r, i, n;
	char name[32];

	// Get handles to src and dest directories.
	err = HGetVol(NULL, &srcVol, &srcDir);
	if (err != 0) {
		print_errcode(err, "HGetVol");
		return 1;
	}
	r = dir_from_path(&destVol, &destDir, destpath);
	if (r != 0) {
		return 1;
	}

	// List files in src and dest directories.
	r = list_dir(srcVol, srcDir, kSrcDir);
	if (r != 0) {
		return 1;
	}
	r = list_dir(destVol, destDir, kDestDir);
	if (r != 0) {
		return 1;
	}
	if (gFileCount == 0) {
		print_err("no files");
		return 1;
	}

	HLock(gFiles);
	array = (struct file_info *)*gFiles;
	n = gFileCount;

	// Figure out the direction for each file.
	srcNewer = NULL;
	destNewer = NULL;
	for (i = 0; i < n; i++) {
		file = &array[i];
		if (!file->meta[kSrcDir].exists) {
			file->mode = kModePull;
			destNewer = file;
		} else if (!file->meta[kDestDir].exists) {
			file->mode = kModePush;
			srcNewer = file;
		} else if (file->meta[kSrcDir].modTime < file->meta[kDestDir].modTime) {
			file->mode = kModePull;
			destNewer = file;
		} else if (file->meta[kSrcDir].modTime > file->meta[kDestDir].modTime) {
			file->mode = kModePush;
			srcNewer = file;
		}
	}

	// Figure out the mode: push or pull.
	if (mode == kModeAuto) {
		if (srcNewer != NULL) {
			if (destNewer != NULL) {
				fputs("## Error: both source and destination have new files\n",
				      stderr);
				p2cstr(name, srcNewer->name);
				fprintf(stderr, "## New file in source: %s\n", name);
				p2cstr(name, destNewer->name);
				fprintf(stderr, "## New file in destination: %s\n", name);
				return 1;
			}
			mode = kModePush;
			fputs("## Mode: push\n", stderr);
		} else if (destNewer != NULL) {
			mode = kModePull;
			fputs("## Mode: pull\n", stderr);
		} else {
			fputs("## No changes.\n", stderr);
			return 0;
		}
	}

	// Synchronize the files.
	tempVol = 0;
	tempDir = 0;
	for (i = 0; i < n; i++) {
		file = &array[i];
		if (file->mode == mode) {
			if (mode == kModePush) {
				// When pushing, we use the destination directory as the
				// temporary folder, to avoid crossing filesystem boundaries on
				// the host.
				r = sync_file(file, mac_to_unix, srcVol, srcDir, destVol,
				              destDir, destVol, destDir,
				              file->meta[kSrcDir].modTime);
			} else {
				if (tempDir == 0) {
					err = FindFolder(destVol, kTemporaryFolderType, TRUE,
					                 &tempVol, &tempDir);
					if (err != 0) {
						print_errcode(err, "could not find temporary folder");
						return 1;
					}
				}
				r = sync_file(file, mac_to_unix, destVol, destDir, srcVol,
				              srcDir, tempVol, tempDir,
				              file->meta[kDestDir].modTime);
			}
			if (r) {
				p2cstr(name, file->name);
				print_err("failed to copy file: %s", name);
				return 1;
			}
		} else if (file->mode != kModeAuto) {
			p2cstr(name, file->name);
			fprintf(stderr, "## Refusing to overwrite '%s', file is newer\n",
			        name);
		}
	}

	return 0;
}

int main(int argc, char **argv) {
	char *destDir = NULL, *arg, *opt;
	int i, r, mode = kModeAuto;

	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if (*arg == '-') {
			opt = arg + 1;
			if (*opt == '-') {
				opt++;
			}
			if (strcmp(opt, "push") == 0) {
				mode = kModePush;
			} else if (strcmp(opt, "pull") == 0) {
				mode = kModePull;
			} else {
				print_err("unknown flag: %s", arg);
				return 1;
			}
		} else {
			if (destDir != NULL) {
				print_err("unexpected argument: %s", arg);
				return 1;
			}
			destDir = arg;
		}
	}

	r = command_main(argv[1], mode);
	if (gFiles != NULL) {
		DisposeHandle(gFiles);
	}
	return r;
}

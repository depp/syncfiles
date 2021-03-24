#include "defs.h"

#include <CursorCtl.h>
#include <Files.h>
#include <Folders.h>
#include <MacMemory.h>
#include <StringCompare.h>

#include <stdio.h>
#include <string.h>

log_level gLogLevel;
bool gFlagForce;
bool gFlagDryRun;
bool gFlagDelete;

static const char *kActionName[] = {
	"no action",
	"new",
	"replace",
	"delete",
};

// Global operation mode
enum {
	kModeUnknown,
	kModePush,
	kModePull,
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
static int filter_name(const unsigned char *name) {
	int len, i, stem;
	const unsigned char *ext;
	char temp[32];
	unsigned char c0, c1, c2;

	if (EqualString(name, "\pmakefile", false, true)) {
		return 1;
	}
	stem = 0;
	len = name[0];
	for (i = 1; i <= len; i++) {
		if (name[i] == '.') {
			stem = i;
		}
	}
	if (stem != 0) {
		ext = name + stem + 1;
		switch (len - stem) {
		case 1:
			// .c .h .r
			c0 = ext[0];
			if (c0 == 'c' || c0 == 'h' || c0 == 'r') {
				return 1;
			}
			break;
		case 2:
			// .cc .cp .hh
			c0 = ext[0];
			c1 = ext[1];
			if (c0 == 'c' && (c1 == 'c' || c1 == 'p') ||
			    c0 == 'h' && c1 == 'h') {
				return 1;
			}
			break;
		case 3:
			// .cpp .hpp .cxx .hxx
			c0 = ext[0];
			c1 = ext[1];
			c2 = ext[2];
			if ((c0 == 'c' || c0 == 'h') &&
			    (c1 == 'p' && c2 == 'p' || c1 == 'x' && c2 == 'x')) {
				return 1;
			}
			break;
		}
	}
	if (gLogLevel >= kLogVerbose) {
		p2cstr(temp, name);
		fprintf(stderr, "## Ignored: %s\n", temp);
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
			file->meta[which].exists = true;
			file->meta[which].modTime = ci.hFileInfo.ioFlMdDat;
		}
	}

	return 0;
}

static int command_main(char *localPath, char *remotePath, int mode) {
	short localVol, remoteVol, srcVol, destVol, tempVol;
	long localDir, remoteDir, srcDir, destDir, tempDir;
	struct file_info *array, *file;
	OSErr err;
	int r, i, n;
	char name[32];
	bool hasAction;
	convert_func func;

	// Get handles to local and remote directories.
	if (localPath == NULL) {
		err = HGetVol(NULL, &localVol, &localDir);
		if (err != 0) {
			print_errcode(err, "HGetVol");
			return 1;
		}
	} else {
		r = dir_from_path(&localVol, &localDir, localPath);
		if (r != 0) {
			return 1;
		}
	}
	r = dir_from_path(&remoteVol, &remoteDir, remotePath);
	if (r != 0) {
		return 1;
	}
	if (localVol == remoteVol && localDir == remoteDir) {
		print_err("local and remote are the same directory");
		return 1;
	}

	// Get source and destination directories.
	if (mode == kModePull) {
		srcVol = remoteVol;
		srcDir = remoteDir;
		destVol = localVol;
		destDir = localDir;
	} else {
		srcVol = localVol;
		srcDir = localDir;
		destVol = remoteVol;
		destDir = remoteDir;
	}

	// List files in src and dest directories.
	r = list_dir(srcVol, srcDir, kSrcDir);
	if (r != 0) {
		return 1;
	}
	if (gFileCount == 0) {
		print_err("no files in source directory");
		return 1;
	}
	r = list_dir(destVol, destDir, kDestDir);
	if (r != 0) {
		return 1;
	}

	HLock(gFiles);
	array = (struct file_info *)*gFiles;
	n = gFileCount;

	// Assign actions to each file.
	hasAction = false;
	for (i = 0; i < n; i++) {
		file = &array[i];
		file->action = kActionNone;
		if (!file->meta[kSrcDir].exists) {
			if (gFlagDelete) {
				file->action = kActionDelete;
				hasAction = true;
			}
		} else if (!file->meta[kDestDir].exists) {
			file->action = kActionNew;
			hasAction = true;
		} else if (gFlagForce ||
		           file->meta[kSrcDir].modTime > file->meta[kDestDir].modTime) {
			file->action = kActionReplace;
			hasAction = true;
		} else if (file->meta[kDestDir].modTime <
		           file->meta[kDestDir].modTime) {
			p2cstr(name, file->name);
			fprintf(stderr, "## Warning: destination file is newer: %s\n",
			        name);
		}
	}

	// Early exit if there are no actions.
	if (!hasAction) {
		fputs("## No actions\n", stderr);
		return 0;
	}

	// Print actions for a dry run.
	if (gFlagDryRun) {
		for (i = 0; i < n; i++) {
			file = &array[i];
			p2cstr(name, file->name);
			fprintf(stderr, "## %s: %s\n", kActionName[file->action], name);
		}
		return 0;
	}

	// Synchronize the files.
	InitCursorCtl(NULL);
	if (mode == kModePull) {
		r = mac_from_unix_init();
		if (r != 0) {
			return 1;
		}
		func = mac_from_unix;
		err =
			FindFolder(destVol, kTemporaryFolderType, true, &tempVol, &tempDir);
		if (err != 0) {
			print_errcode(err, "could not find temporary folder");
			return 1;
		}
	} else {
		func = mac_to_unix;
		// When pushing, we use the destination directory as the temporary
		// folder, to avoid crossing filesystem boundaries on the host.
		tempVol = destVol;
		tempDir = destDir;
	}
	for (i = 0; i < n; i++) {
		file = &array[i];
		p2cstr(name, file->name);
		if (gLogLevel >= kLogInfo) {
			fprintf(stderr, "## %s: %s\n", kActionName[file->action], name);
		}
		SpinCursor(32);
		r = sync_file(&array[i], func, srcVol, srcDir, destVol, destDir,
		              tempVol, tempDir);
		if (r != 0) {
			print_err("failed to synchronize file: %s", name);
			return 1;
		}
	}

	return 0;
}

int main(int argc, char **argv) {
	char *remoteDir = NULL, *localDir = NULL, *arg, *opt;
	int i, r, mode = kModeUnknown;

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
			} else if (strcmp(opt, "verbose") == 0 || strcmp(opt, "v") == 0) {
				gLogLevel = kLogVerbose;
			} else if (strcmp(opt, "quiet") == 0 || strcmp(opt, "q") == 0) {
				gLogLevel = kLogWarn;
			} else if (strcmp(opt, "force") == 0 || strcmp(opt, "f") == 0) {
				gFlagForce = true;
			} else if (strcmp(opt, "dir") == 0) {
				if (i + 1 >= argc) {
					print_err("expected argument for -dir");
					return 1;
				}
				localDir = argv[++i];
			} else if (strcmp(opt, "dry-run") == 0 || strcmp(opt, "n") == 0) {
				gFlagDryRun = true;
			} else if (strcmp(opt, "delete") == 0 || strcmp(opt, "d") == 0) {
				gFlagDelete = true;
			} else {
				print_err("unknown flag: %s", arg);
				return 1;
			}
		} else {
			if (remoteDir != NULL) {
				print_err("unexpected argument: %s", arg);
				return 1;
			}
			remoteDir = arg;
		}
	}
	if (mode == kModeUnknown) {
		print_err("either -push or -pull is required");
		return 1;
	}

	r = command_main(localDir, remoteDir, mode);
	if (gFiles != NULL) {
		DisposeHandle(gFiles);
	}
	mac_from_unix_term();
	if (gLogLevel >= kLogVerbose) {
		fputs("## Done\n", stderr);
	}
	return r;
}

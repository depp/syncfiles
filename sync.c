#include <Files.h>
#include <MacMemory.h>
#include <StringCompare.h>

#include <stdio.h>
#include <string.h>

static int c2pstr(Str255 ostr, const char *istr) {
	size_t n = strlen(istr);
	if (n > 255) {
		fprintf(stderr, "## Error: path too long: %s\n", istr);
		return 1;
	}
	ostr[0] = n;
	memcpy(ostr + 1, istr, n);
	memset(ostr + 1 + n, 0, 255 - n);
	return 0;
}

struct file_meta {
	Boolean exists;
	long modTime;
};

enum {
	kSrcDir,
	kDestDir,
};

struct file_info {
	Str31 name;
	struct file_meta meta[2];
};

static Handle gFiles;
static unsigned gFileCount;
static unsigned gFileAlloc;

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
		fprintf(stderr, "## Error: FSMakeFSSpec: %d\n", err);
		return 1;
	}
	memset(&ci, 0, sizeof(ci));
	ci.dirInfo.ioNamePtr = spec.name;
	ci.dirInfo.ioVRefNum = spec.vRefNum;
	ci.dirInfo.ioDrDirID = spec.parID;
	err = PBGetCatInfoSync(&ci);
	if (err != 0) {
		fprintf(stderr, "## Error: PBGetCatInfoSync: %d\n", err);
		return 1;
	}
	if ((ci.dirInfo.ioFlAttrib & kioFlAttribDirMask) == 0) {
		fprintf(stderr, "## Error: not a directory: %s\n", dirpath);
		return 1;
	}
	*vRefNum = ci.dirInfo.ioVRefNum;
	*dirID = ci.dirInfo.ioDrDirID;
	return 0;
}

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
			fprintf(stderr, "## Error: could not list directory (err = %d)\n",
			        err);
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

static int command_main(char *destpath) {
	short srcVol, destVol;
	long srcDir, destDir;
	struct file_info *array, *file;
	OSErr err;
	int r, i, n;
	char name[32];

	err = HGetVol(NULL, &srcVol, &srcDir);
	if (err != 0) {
		fprintf(stderr, "## Error: HGetVol: %d\n", err);
		return 1;
	}
	r = dir_from_path(&destVol, &destDir, destpath);
	if (r != 0) {
		return 1;
	}
	r = list_dir(srcVol, srcDir, kSrcDir);
	if (r != 0) {
		return 1;
	}
	r = list_dir(destVol, destDir, kDestDir);
	if (r != 0) {
		return 1;
	}
	if (gFileCount == 0) {
		fputs("## Error: no files\n", stderr);
		return 1;
	}
	HLock(gFiles);
	array = (struct file_info *)*gFiles;
	n = gFileCount;
	for (i = 0; i < n; i++) {
		file = &array[i];
		memcpy(name, file->name + 1, file->name[0]);
		name[file->name[0]] = '\0';
		printf("File: %s\n", name);
		printf("  exist: %c %c\n", file->meta[0].exists ? 'Y' : '-',
		       file->meta[1].exists ? 'Y' : '-');
		printf("  modTime: %ld %ld\n", file->meta[0].modTime,
		       file->meta[1].modTime);
	}
	HUnlock(gFiles);

	return 0;
}

int main(int argc, char **argv) {
	int r;

	if (argc != 2) {
		fputs("## Usage: SyncFiles <directory>\n", stderr);
		return 1;
	}
	r = command_main(argv[1]);
	if (gFiles != NULL) {
		DisposeHandle(gFiles);
	}
	return r;
}

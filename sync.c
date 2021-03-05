#include <Files.h>
#include <MacMemory.h>

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

static int list_dir(const char *dirpath) {
	Str255 ppath;
	CInfoPBRec ci;
	OSErr err;
	short vRefNum;
	long dirID;
	int i;

	if (c2pstr(ppath, dirpath)) {
		return 1;
	}
	memset(&ci, 0, sizeof(ci));
	ci.dirInfo.ioNamePtr = ppath;
	err = PBGetCatInfoSync(&ci);
	if (err != 0) {
		fprintf(stderr, "## Error: PBGetCatInfoSync: %d\n", err);
		return 1;
	}
	vRefNum = ci.dirInfo.ioVRefNum;
	dirID = ci.dirInfo.ioDrDirID;
	printf("vRefNum: %d; dirID: %d\n", vRefNum, dirID);
	if ((ci.dirInfo.ioFlAttrib & kioFlAttribDirMask) == 0) {
		fprintf(stderr, "## Error: not a directory: %s\n", dirpath);
		return 1;
	}

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
			fprintf(stderr, "## Error: could not list directory '%s' (err = %d)\n", dirpath, err);
			continue;
		}
		ppath[ppath[0] + 1] = '\0';
		printf("    File: '%s'\n", ppath + 1);
	}

	return 0;
}

int main(int argc, char **argv, char **envp) {
	int i;
	(void)envp;
	for (i = 1; i < argc; i++) {
		if (list_dir(argv[i])) {
			return 1;
		}
	}
	return 0;
}

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
	unsigned char path[257];
	WDPBRec wd;
	CInfoPBRec ci;
	OSErr err;
	int i, result = 0;

	if (c2pstr(path, dirpath)) {
		return 1;
	}
	memset(&wd, 0, sizeof(wd));
	wd.ioNamePtr = path;
	err = PBOpenWDSync(&wd);
	if (err != 0) {
		fprintf(stderr, "## Error: could not open directory '%s' (err = %d)\n", dirpath, err);
		return 1;
	}
	printf("Path: %s\n", dirpath);
	printf("    VREF: %d\n", wd.ioVRefNum);

	for (i = 1; i < 10; i++) {
		memset(&ci, 0, sizeof(ci));
		ci.dirInfo.ioNamePtr = path;
		ci.dirInfo.ioVRefNum = wd.ioVRefNum;
		ci.dirInfo.ioFDirIndex = i;
		err = PBGetCatInfoSync(&ci);
		if (err != 0) {
			printf("    err\n");
			if (err != fnfErr) {
				result = 1;
				fprintf(stderr, "## Error: could not list directory '%s' (err = %d)\n", dirpath, err);
			}
			continue;
		}
		path[path[0] + 1] = '\0';
		printf("    File: '%s'\n", path + 1);
	}

	err = PBCloseWDSync(&wd);
	if (err != 0) {
		fprintf(stderr, "## Error: could not close directory '%s' (err = %d)\n", dirpath, err);
		return 1;
	}
	return result;
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

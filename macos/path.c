#include "path.h"

#include "error.h"

#include <string.h>

enum {
	kMaxPath = 512,
};

OSErr GetDirPath(FSSpec *spec, int *pathLength, Handle *path)
{
	Handle segments; // Packed sequence of segments, Pascal strings.
	int textLen;     // Total length of segments, including length byte.
	Handle result;
	unsigned char *name;
	CInfoPBRec ci;
	OSErr err;
	unsigned char *op;
	const unsigned char *ip, *ie;
	int length;

	// Query target.
	memset(&ci, 0, sizeof(ci));
	ci.dirInfo.ioNamePtr = spec->name;
	ci.dirInfo.ioVRefNum = spec->vRefNum;
	ci.dirInfo.ioDrDirID = spec->parID;
	err = PBGetCatInfoSync(&ci);
	if (err != noErr) {
		return err;
	}
	// FIXME: kioFlAttribDirMask
	if ((ci.hFileInfo.ioFlAttrib & ioDirMask) == 0) {
		return fnfErr;
	}
	segments = NULL;
	textLen = spec->name[0] + 1;
	PtrToHand(spec->name, &segments, textLen);
	if (segments == NULL) {
		return MemError();
	}

	// Add name of parent.
	while (ci.dirInfo.ioDrParID != fsRtParID) {
		ASSERT(textLen < kMaxPath);
		SetHandleSize(segments, textLen + 256);
		err = MemError();
		if (err != noErr) {
			goto done;
		}
		HLock(segments);
		name = (unsigned char *)*segments + textLen;
		ci.dirInfo.ioNamePtr = name;
		ci.dirInfo.ioDrDirID = ci.dirInfo.ioDrParID;
		ci.dirInfo.ioFDirIndex = -1;
		err = PBGetCatInfoSync(&ci);
		HUnlock(segments);
		if (err != 0) {
			goto done;
		}
		textLen += name[0] + 1;
	}

	// Copy segments into handle in reverse order.
	result = NewHandle(textLen);
	if (result == NULL) {
		err = MemError();
		goto done;
	}
	op = (unsigned char *)*result + textLen;
	ip = (const unsigned char *)*segments;
	ie = ip + textLen;
	while (ip < ie) {
		op--;
		*op = ':';
		length = *ip;
		op -= length;
		BlockMoveData(ip + 1, op, length);
		ip += length + 1;
	}
	*pathLength = textLen;
	*path = result;
	err = 0;

done:
	if (segments != NULL) {
		DisposeHandle(segments);
	}
	return err;
}

#ifndef MACOS_TEMPFILE_H
#define MACOS_TEMPFILE_H

// Create a FSSpec for a temporary file on the given volume. Does not actually
// create the file.
OSErr MakeTempFile(short vRefNum, FSSpec *spec);

#endif

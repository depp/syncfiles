// path.h - Path manipulation utilities.
#ifndef MACOS_PATH_H
#define MACOS_PATH_H

// Get the full path to a directory.
OSErr GetDirPath(FSSpec *spec, int *pathLength, Handle *path);

#endif

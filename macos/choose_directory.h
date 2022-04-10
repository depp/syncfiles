#ifndef MACOS_CHOOSE_DIRECTORY_H
#define MACOS_CHOOSE_DIRECTORY_H

// Present a dialog box to choose a directory. Returns true and initializes
// directory if a directory is selected.
Boolean ChooseDirectory(FSSpec *directory);

#endif

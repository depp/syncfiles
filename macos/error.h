// error.h - Error handling.
#ifndef MACOS_ERROR_H
#define MACOS_ERROR_H

// Error codes, corresponding to messages in a STR# resource.
typedef enum {
	kErrUnknown = 1,
	kErrInternal,
	kErrOutOfMemory,
	kErrCouldNotSaveProject,
} ErrorCode;

struct Error {
	ErrorCode code;
	short osErr;
};

// Show an error dialog with the given error message, then quit the program.
void ExitError(ErrorCode errCode);

// Show an error dialog with the given error message and an OS error code, then
// quit the program.
void ExitErrorOS(ErrorCode errCode, short osErr);

// Show an out of memory error and quit the program.
void ExitMemError(void);

// Show an assertion error and quit the program.
void ExitAssert(const unsigned char *file, int line,
                const unsigned char *message);

#define EXIT_ASSERT(str) ExitAssert("\p" __FILE__, __LINE__, str)

#define ASSERT(p)                                         \
	do {                                                  \
		if (!(p))                                         \
			ExitAssert("\p" __FILE__, __LINE__, "\p" #p); \
	} while (0)

struct ErrorParams {
	ErrorCode err;
	OSErr osErr;
	const unsigned char *strParam;
};

void ShowError(const struct ErrorParams *p);

#endif

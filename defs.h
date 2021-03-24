#include <MacTypes.h>

typedef unsigned char bool;

// =============================================================================
// util.c
// =============================================================================

// Message log level.
typedef enum {
	kLogWarn,
	kLogInfo,
	kLogVerbose,
} log_level;

// Global log verbosity.
extern log_level gLogLevel;

// -f / -force flag: if true, all destination files are replaced, regardless of
// timestamp.
extern bool gFlagForce;

// -n / -dry-run flag: if true, no actions are taken, but the actions are
// printed out.
extern bool gFlagDryRun;

// -d / -delete flag: if true, destination files are deleted if there is no
// corresponding source file.
extern bool gFlagDelete;

// Print an error message.
void print_err(const char *msg, ...);

// Print an error message with a Macintosh toolbox error code.
void print_errcode(OSErr err, const char *msg, ...);

// Print an out-of-memory error.
void print_memerr(unsigned long size);

// Print an abort message.
void print_abort_func(const char *file, int line);

#define print_abort() print_abort_func(__FILE__, __LINE__)

// Log the error result of a function call.
void log_call(OSErr err, const char *function);

// Convert a C string to Pascal string. Returns nonzero on failure.
int c2pstr(unsigned char *ostr, const char *istr);

// Convert a Pascall string (maximum 31 characters) to a C string.
void p2cstr(char *ostr, const unsigned char *istr);

// =============================================================================
// file.c
// =============================================================================

// Text file conversion function.
typedef int (*convert_func)(short src, short dest, void *srcBuf, void *destBuf);

enum {
	kSrcDir,
	kDestDir,
};

// Metadata for a file in the source or destination.
struct file_meta {
	Boolean exists;
	long modTime;
};

// An action to take for a particular file.
typedef enum {
	kActionNone,    // Leave file alone.
	kActionNew,     // Copy src to dest, dest does not exist.
	kActionReplace, // Replace existing file in dest.
	kActionDelete,  // Delete dest file.
} file_action;

// Information about a file present in the source or destination directory (or
// both).
struct file_info {
	// Filename, Pascal string.
	Str31 name;
	// Metadata indexed by kSrcDir or kDestDir.
	struct file_meta meta[2];
	// The action to apply to this file.
	file_action action;
};

// Synchronize a file according to the action in the action field. The temporary
// directory must be a valid directory on the destination volume.
int sync_file(struct file_info *file, convert_func func, short srcVol,
              long srcDir, short destVol, long destDir, short tempVol,
              long tempDir);

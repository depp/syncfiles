// These helper functions are written so the conversion functions can be written
// for a standard C environment without using Macintosh Toolbox functions.

enum {
	// Base size of temporary buffer for converting files, not counting the
	// "extra".
	kBufferBaseSize = 16 * 1024,

	// Extra space past the end of the buffer for converting files.
	kBufferExtraSize = 16,

	// Total size of a buffer.
	kBufferTotalSize = kBufferBaseSize + kBufferExtraSize,
};

// =============================================================================
// Helper functions
// =============================================================================

// Result codes for convert_read and convert_write.
enum {
	kConvertOK,
	kConvertError,
	kConvertEOF,
};

// Read data from a file.
int convert_read(short ref, long *count, void *data);

// Write data to a file.
int convert_write(short ref, long count, const void *data);

// Get the table for converting from Unix to Macintosh.
unsigned short *mac_from_unix_data(void);

// =============================================================================
// Conversion functions
// =============================================================================

// Convert Macintosh encoding with CR line endings to UTF-8 with LF. The source
// and destinations are file handles. The buffers have size buf
int mac_to_unix(short srcRef, short destRef, void *srcBuf, void *destBuf);

// Convert UTF-8 with LF line endings to Macintosh encoding with CR. The source
// and destinations are file handles.  The buffers have size kBufferTotalSize.
int mac_from_unix(short srcRef, short destRef, void *srcBuf, void *destBuf);

// Raw data copy.
int copy_data(short srcRef, short destRef, void *buf);

// Convert line endings but don't change encoding.
int convert_line_endings(short srcRef, short destRef, void *buf,
                         unsigned char srcEnding, unsigned char destEnding);

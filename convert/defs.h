#ifndef defs_h
#define defs_h
/* defs.h - common definitions. */

#if macintosh

#include <MacTypes.h>

#elif USE_CARBON

/* We get definitions from pre-compiled headers. */

#else

/* Definitions in <MacTypes.h> */

/* Include <stddef.h> for NULL */
#include <stddef.h>
#include <stdint.h>

typedef uint8_t UInt8;
typedef int8_t SInt8;
typedef uint16_t UInt16;
typedef int16_t SInt16;
typedef uint32_t UInt32;
typedef int32_t SInt32;

typedef char *Ptr;
typedef Ptr *Handle;
typedef long Size;

typedef SInt16 OSErr;

/* Definitions in <MacMemory.h> */

Handle NewHandle(Size byteCount);
void HLock(Handle h);
void HUnlock(Handle h);
void DisposeHandle(Handle h);
void SetHandleSize(Handle h, Size newSize);
OSErr MemError(void);

#endif

void MemClear(void *ptr, Size size);

#endif

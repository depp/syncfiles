#ifndef defs_h
#define defs_h
/* defs.h - common definitions. */

/*==============================================================================
Basic definitions
==============================================================================*/

#if macintosh

#include <MacTypes.h>

#else

/* Include <stddef.h> for NULL */
#include <stddef.h>
#include <stdint.h>

#if __STDC_VERSION__ >= 199901l
#include <stdbool.h>
#endif

#if __bool_true_false_are_defined
typedef bool Boolean;
#else
enum
{
	false,
	true
};
typedef unsigned char Boolean;
#endif

typedef uint8_t UInt8;
typedef int8_t SInt8;
typedef uint16_t UInt16;
typedef int16_t SInt16;
typedef uint32_t UInt32;
typedef int32_t SInt32;

typedef char *Ptr;
typedef Ptr *Handle;
typedef long Size;

#endif

/*==============================================================================
Error codes and error reporting
==============================================================================*/

/* Error codes. */
typedef enum
{
	/* No error. */
	kErrorOK,

	/* Memory allocation failed. */
	kErrorNoMemory,

	/* Invaild table data. */
	kErrorBadData
} ErrorCode;

/*==============================================================================
Memory allocation
==============================================================================*/

#if macintosh

#include <MacMemory.h>

#else

/* Allocate a relocatable block of memory. */
Handle NewHandle(Size byteSize);

/* Free a relocatable block of memory. */
void DisposeHandle(Handle h);

#endif

/* Resize a relocatable block of memory. Return true on success. */
Boolean ResizeHandle(Handle h, Size newSize);

/* Fill memory with zeroes. */
void MemClear(void *ptr, Size size);

#endif

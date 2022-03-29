#ifndef lib_defs_h
#define lib_defs_h
/* defs.h - common definitions. */

/*==============================================================================
Target information
==============================================================================*/

/*
  Note that this code does not need to be especially portable. It just runs on
  Mac OS and development systems (for testing). We can assume that the
  development system has GCC.

  Macros we care about:

  TARGET_OS_MAC             OS is some Macintosh variant (broadly speaking)
  TARGET_API_MAC_OS8        Targeting classic Mac OS (9.x and earlier)

  TARGET_RT_LITTLE_ENDIAN   Little-endian byte order
  TARGET_RT_BIG_ENDIAN      Big-endian byte order
*/
#if macintosh

/* Classic Mac OS. Header is part of Universal Interfaces & Carbon. */
#include <ConditionalMacros.h>

#elif __APPLE__

/* Newer apple systems, including macOS >= 10. Header is in /usr/include, or
   within /usr/include inside the SDK. */
#include <TargetConditionals.h>

#else

#if __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TARGET_RT_LITTLE_ENDIAN 1
#define TARGET_RT_BIG_ENDIAN 0
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define TARGET_RT_LITTLE_ENDIAN 0
#define TARGET_RT_BIG_ENDIAN 1
#else
#error "unknown endian"
#endif
#else
#error "could not determine byte order"
#endif

#endif

/*==============================================================================
Basic types
==============================================================================*/

#if TARGET_API_MAC_OS8

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

#if TARGET_API_MAC_OS8

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

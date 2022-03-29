#ifndef lib_endian_h
#define lib_endian_h
/* endian.h - byte order and byte swapping */

/*
  Defines macros for swapping to and from big-endian:

  EndianU16_BtoN
  EndianU16_NtoB
  EndianU32_BtoN
  EndianU32_NtoB
*/

#include "lib/defs.h"

#if TARGET_API_MAC_OS8

/* Use the legacy Mac OS header, if available. */
#include <Endian.h>

#else

/* Use the compiler built-in, if available. */
#if __clang__

#if __has_builtin(__builtin_bswap16)
#define Endian16_Swap(x) __builtin_bswap16(x)
#endif

#if __has_builtin(__builtin_bswap32)
#define Endian32_Swap(x) __builtin_bswap32(x)
#endif

#elif __GNUC__

#if __GNUC__ && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define Endian16_Swap(x) __builtin_bswap16(x)
#define Endian32_Swap(x) __builtin_bswap32(x)
#endif

#endif

/* Fallback implementations. */

/* clang-format off */
#ifndef Endian16_Swap
#define Endian16_Swap(x)             \
	((((UInt16)(x) << 8) & 0xff00) | \
	 (((UInt16)(x) >> 8) & 0x00ff))
#endif

#ifndef Endian32_Swap
#define Endian32_Swap(x)                  \
	((((UInt32)(x) << 24) & 0xff000000) | \
	 (((UInt32)(x) <<  8) & 0x00ff0000) | \
	 (((UInt32)(x) >>  8) & 0x0000ff00) | \
	 (((UInt32)(x) >> 24) & 0x000000ff))
#endif
/* clang-format on */

#if TARGET_RT_BIG_ENDIAN
#define EndianU16_BtoN(x) (x)
#define EndianU16_NtoB(x) (x)
#define EndianU32_BtoN(x) (x)
#define EndianU32_NtoB(x) (x)
#else
#define EndianU16_BtoN(x) Endian16_Swap(x)
#define EndianU16_NtoB(x) Endian16_Swap(x)
#define EndianU32_BtoN(x) Endian32_Swap(x)
#define EndianU32_NtoB(x) Endian32_Swap(x)
#endif

#endif /* TARGET_API_MAC_OS8 */

#endif /* lib_endian_h */

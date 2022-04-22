// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#ifndef CONVERT_CONVERT_H
#define CONVERT_CONVERT_H
/* convert.h - character set conversion routines. */

#include "lib/defs.h"
#include "lib/error.h"

enum {
	/* Constants for CR and LF. Note that we should not use '\n' or '\r'
	   anywhere, because these character constants may have unexpected values on
	   certain old Mac OS compilers, depending on the compiler settings. In
	   particular, the values of '\n' and '\r' will be swapped. */
	kCharLF = 10,
	kCharCR = 13,

	/* Constant for substitution character: '?'. */
	kCharSubstitute = 63
};

typedef enum {
	/* Don't translite line breaks. */
	kLineBreakKeep,

	/* Convert line breaks to LF. */
	kLineBreakLF,

	/* Convert line breaks to CR. */
	kLineBreakCR,

	/* Convert line breaks to CR LF. */
	kLineBreakCRLF
} LineBreakConversion;

/* Directions that the converter runs in. */
typedef enum { kToUTF8, kFromUTF8 } ConvertDirection;

/* Get the character map used for the given Mac OS script and region codes.
   Return -1 if no known character map exists. */
int GetCharmap(int script, int region);

/* The state of a converter. Must be zeroed prior to first conversion. */
struct ConverterState {
	UInt32 data;
};

/* Implementation function for building a converter. */
typedef ErrorCode (*ConvertBuildf)(Handle *out, Handle data, Size datasz);

/* Implementation function for running a converter. */
typedef void (*ConvertRunf)(const void *cvtptr, LineBreakConversion lc,
                            struct ConverterState *stateptr, UInt8 **optr,
                            UInt8 *oend, const UInt8 **iptr, const UInt8 *iend);

/* A converter. The converter can be freed by disposing the handle. */
struct Converter {
	Handle data;
	ConvertRunf run;
};

/* Build a converter from the given conversion table data. */
int ConverterBuild(struct Converter *c, Handle data, Size datasz,
                   ConvertDirection direction);

/* Engine 1: extended ASCII */

ErrorCode Convert1fBuild(Handle *out, Handle data, Size datasz);
void Convert1fRun(const void *cvtptr, LineBreakConversion lc,
                  struct ConverterState *stateptr, UInt8 **optr, UInt8 *oend,
                  const UInt8 **iptr, const UInt8 *iend);

ErrorCode Convert1rBuild(Handle *out, Handle data, Size datasz);
void Convert1rRun(const void *cvtptr, LineBreakConversion lc,
                  struct ConverterState *stateptr, UInt8 **optr, UInt8 *oend,
                  const UInt8 **iptr, const UInt8 *iend);

#endif

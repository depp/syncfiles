#ifndef convert_h
#define convert_h
/* convert.h - character set conversion routines. */

/* Get the character map used for the given Mac OS script and region codes.
   Return -1 if no known character map exists. */
int GetCharmap(int script, int region);

#endif

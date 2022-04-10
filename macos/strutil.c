#include "strutil.h"

#include "error.h"

#include <NumberFormatting.h>

#include <stdarg.h>
#include <string.h>

static unsigned char *StrPtrAppend(unsigned char *ptr, unsigned char *end,
                                   const unsigned char *str)
{
	unsigned i, len, rem;

	len = str[0];
	rem = end - ptr;
	if (len > rem) {
		len = rem;
	}
	str++;
	for (i = 0; i < len; i++) {
		*ptr++ = *str++;
	}
	return ptr;
}

static void StrAppendVFormat(unsigned char *dest, const unsigned char *msg,
                             va_list ap)
{
	const unsigned char *mptr = msg + 1, *mend = mptr + msg[0];
	unsigned char *ptr = dest + 1 + dest[0], *end = dest + 256;
	unsigned char c, temp[16];
	const unsigned char *pstrvalue;
	int ivalue;

next:
	while (mptr < mend) {
		c = *mptr++;
		if (c == '%' && mptr < mend) {
			switch (*mptr) {
			case '%':
				mptr++;
				break;
			case 'd':
				mptr++;
				ivalue = va_arg(ap, int);
				NumToString(ivalue, temp);
				ptr = StrPtrAppend(ptr, end, temp);
				goto next;
			case 'S':
				mptr++;
				pstrvalue = va_arg(ap, const unsigned char *);
				ptr = StrPtrAppend(ptr, end, pstrvalue);
				goto next;
			}
		}
		if (ptr == end) {
			break;
		}
		*ptr++ = c;
	}

	*dest = ptr - dest - 1;
}

void StrFormat(unsigned char *dest, const unsigned char *msg, ...)
{
	va_list ap;

	dest[0] = 0;
	va_start(ap, msg);
	StrAppendVFormat(dest, msg, ap);
	va_end(ap);
}

void StrAppendFormat(unsigned char *dest, const unsigned char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	StrAppendVFormat(dest, msg, ap);
	va_end(ap);
}

void StrCopy(unsigned char *dest, int dest_size, unsigned char *src)
{
	int len = src[0] + 1;
	if (len > dest_size) {
		EXIT_ASSERT(NULL);
	}
	BlockMoveData(src, dest, len);
}

struct StrRef {
	const unsigned char *src;
	int len;
};

void StrSubstitute(unsigned char *str, const unsigned char *param)
{
	int i, n, end;
	struct StrRef refs[3];

	n = str[0];
	for (i = 0; i < n - 1; i++) {
		if (str[i + 1] == '^' && str[i + 2] == '1') {
			refs[0].src = str + 1;
			refs[0].len = i;
			refs[1].src = param + 1;
			refs[1].len = param[0];
			refs[2].src = str + i + 3;
			refs[2].len = n - i - 2;
			goto concat;
		}
	}
	return;

concat:
	n = 0;
	for (i = 0; i < 3; i++) {
		n += refs[i].len;
	}
	str[0] = n > 255 ? 255 : n;
	for (i = 3; i > 0;) {
		i--;
		end = n;
		n -= refs[i].len;
		if (end > 255) {
			end = 255;
		}
		if (n < end) {
			memmove(str + 1 + n, refs[i].src, end - n);
		}
	}
}

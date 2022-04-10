#include "lib/crc32.h"

#include <stdio.h>

static const char kTest[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

int main(int argc, char **argv)
{
	UInt32 expect;
	UInt32 val;

	(void)argc;
	(void)argv;

	expect = 0xcbf43926;
	val = CRC32Update(0, kTest, sizeof(kTest));
	if (val != expect) {
		fprintf(stderr, "Error: CRC = 0x%08x, expect 0x%08x\n", val, expect);
		return 1;
	}
	return 0;
}

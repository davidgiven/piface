/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

static void dump_cb(int argc, const char* argv[])
{
	struct file* fp;
	uint32_t base;
	uint32_t offset;
	uint8_t buffer[16];

	if (argc != 2)
	{
		setError("syntax: dump <filename>");
		return;
	}

	fp = vfs_open(argv[1], O_RDONLY);
	if (!fp)
		return;

	vfs_info(fp, &base, NULL);
	offset = 0;
	for (;;)
	{
		int i;
		int r = vfs_read(fp, offset, buffer, 16);
		printf("%08x : ", offset + base);

		for (i=0; i<r; i++)
			printf("%02x ", buffer[i]);
		for (i=r; i<16; i++)
			printf("   ");

		printf(": ");

		for (i=0; i<r; i++)
		{
			uint8_t c = buffer[i];
			if ((c <= 32) || (c >= 127))
				c = '.';
			putchar(c);
		}
		printf("\n");

		if (r != 16)
			break;
		offset += 16;
	}

	vfs_close(fp);
}

const struct command dump_cmd =
{
	"dump",
	"hex-dumps a file",

	"Syntax:\n"
	"  dump <filename>\n"
	"To dump memory, use a mem: file, e.g.:\n"
	"  dump mem:80000000+ff",

	dump_cb
};



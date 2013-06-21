/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

#define BUFFER_SIZE 512

static void cp_cb(int argc, const char* argv[])
{
	struct file* srcfile = NULL;
	struct file* destfile = NULL;
	char* buffer = NULL;
	uint32_t len;
	uint32_t offset;

	if (argc != 3)
	{
		setError("syntax: cp <srcfile> <destfile>");
		return;
	}

	srcfile = vfs_open(argv[1], O_RDONLY);
	if (!srcfile)
		goto exit;

	destfile = vfs_open(argv[2], O_WRONLY);
	if (!destfile)
		goto exit;

	buffer = malloc(BUFFER_SIZE);
	vfs_info(srcfile, NULL, &len);
	offset = 0;

	for (;;)
	{
		uint32_t w;
		uint32_t r = len - offset;
		if (r == 0)
			break;
		if (r > BUFFER_SIZE)
			r = BUFFER_SIZE;

		r = vfs_read(srcfile, offset, buffer, r);
		printf("read %d from 0x%08x\n", r, offset);

		w = 0;
		while (w < r)
		{
			uint32_t i = vfs_write(destfile, offset, buffer+w, r-w);
			printf("written %d to 0x%08x\n", i, offset);
			w += i;
			offset += i;
		}
	}

exit:
	if (buffer)
		free(buffer);
	if (srcfile)
		vfs_close(srcfile);
	if (destfile)
		vfs_close(destfile);
}

const struct command cp_cmd =
{
	"cp",
	"copies one file to another",

	"Syntax:\n"
	"  cp <srcfile> <destfile>\n"
	"Copies a file. Any filesystem scheme can be used. Wildcards are not\n"
	"supported!",

	cp_cb
};

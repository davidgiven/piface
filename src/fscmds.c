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
	uint32_t prevoffset;

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

	prevoffset = 0;
	for (;;)
	{
		uint32_t w;
		uint32_t r = len - offset;
		if (r == 0)
			break;
		if (r > BUFFER_SIZE)
			r = BUFFER_SIZE;

		r = vfs_read(srcfile, offset, buffer, r);

		w = 0;
		while (w < r)
		{
			uint32_t i = vfs_write(destfile, offset, buffer+w, r-w);
			w += i;
			offset += i;
		}

		if ((offset - prevoffset) >= (16*1024))
		{
			printf("%d kB\r", offset/1024);
			fflush(stdout);
			prevoffset = offset;
		}
	}
	printf("\n");

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

static void ls_enumerate_cb(const char* path, int isdir, uint32_t len)
{
	if (isdir)
		printf("       [DIR] ");
	else
		printf("  %10d ", len);
	printf("%s\n", path);
}

static void ls_cb(int argc, const char* argv[])
{
	if (argc != 2)
	{
		setError("syntax: ls <path>");
		return;
	}

	vfs_enumerate(argv[1], ls_enumerate_cb);
}

const struct command ls_cmd =
{
	"ls",
	"lists files available at a path",

	"Syntax:\n"
	"  ls <path>\n"
	"Lists files available at a particular path (if the file system supports\n"
	"it).",

	ls_cb
};

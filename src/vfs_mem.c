/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

struct memfile
{
	uint32_t start;
	uint32_t length;
};

static void* open_cb(const char* path, int flags);
static void close_cb(void* backend);
static uint32_t read_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
static uint32_t write_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length);

const struct filecbs filecbs_mem =
{
	close_cb,
	read_cb,
	write_cb
};

const struct vfs vfs_mem =
{
	"mem",
	&filecbs_mem,

	open_cb
};

static void malformed(void)
{
	setError("malformed mem: path (use <start> or <start>+<len>)");
}

static void* open_cb(const char* path, int flags)
{
    uint32_t start, length;
    char dummy;
    struct memfile* fp;
    int i = sscanf(path, "%x+%x%c", &start, &length, &dummy);


    if ((i != 1) && (i != 2))
    {
        malformed();
        return NULL;
    }

	if (i == 1)
	{
		i = sscanf(path, "%x%c", &start, &dummy);
		if (i != 1)
		{
			malformed();
			return NULL;
		}

		if (flags == O_RDONLY)
		{
			setError("mem: paths with no length cannot be used for reading");
			return NULL;
		}
		length = 0xffffffff - start;
	}

	fp = malloc(sizeof(struct memfile));
	fp->start = start;
	fp->length = length;
	return fp;
}

static void close_cb(void* backend)
{
	struct memfile* fp = backend;
	free(fp);
}

static uint32_t read_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length)
{
	struct memfile* fp = backend;
	uint32_t s = fp->start + offset;

	if (offset > fp->length)
		return 0;
	if ((offset+length) > fp->length)
		length = fp->length - offset;

	memcpy(buffer, (void*)s, length);
	return length;
}

static uint32_t write_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length)
{
	struct memfile* fp = backend;
	uint32_t s = fp->start + offset;

	if (offset > fp->length)
		return 0;
	if ((offset+length) > fp->length)
		length = fp->length - offset;

	memcpy((void*)s, buffer, length);
}


/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

#if defined TARGET_TESTBED

static void* open_cb(const char* path, int flags);
static void close_cb(void* backend);
static uint32_t read_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
static uint32_t write_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
static void info_cb(void* backend,
		uint32_t* base, uint32_t* length);

const struct filecbs filecbs_host =
{
	close_cb,
	read_cb,
	write_cb,
	info_cb
};

const struct vfs vfs_host =
{
	"host",
	&filecbs_host,

	open_cb
};

static void* open_cb(const char* path, int flags)
{
	FILE* fp = fopen(path, (flags==O_RDONLY) ? "r" : "w+");
	if (!fp)
		setError("host error %d", errno);
	return fp;
}

static void close_cb(void* backend)
{
	FILE* fp = backend;
	fclose(fp);
}

static uint32_t read_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length)
{
	FILE* fp = backend;
    fseek(fp, offset, SEEK_SET);
	return fread(buffer, 1, length, fp);
}

static uint32_t write_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length)
{
	FILE* fp = backend;
    fseek(fp, offset, SEEK_SET);
	return fwrite(buffer, 1, length, fp);
}

static void info_cb(void* backend,
		uint32_t* base, uint32_t* length)
{
	FILE* fp = backend;
	*base = 0;
	fseek(fp, 0, SEEK_END);
	*length = ftell(fp);
}

#endif


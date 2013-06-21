/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"
#include "ff.h"

static FATFS fatfs;
static int inited = 0;

static void* open_cb(const char* path, int flags);
static void close_cb(void* backend);
static uint32_t read_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
static uint32_t write_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
static void info_cb(void* backend,
		uint32_t* base, uint32_t* length);

const struct filecbs filecbs_sd =
{
	close_cb,
	read_cb,
	write_cb,
	info_cb
};

const struct vfs vfs_sd =
{
	"sd",
	&filecbs_sd,

	open_cb
};

static const char* error_strings[] =
{
	"success",
	"I/O error",
	"internal error",
	"drive not ready",
	"file not found",
	"path not found",
	"invalid path",
	"access denied",
	"prohibited access",
	"invalid object",
	"write protected",
	"invalid drive",
	"not enabled",
	"no filesystem",
	"mkfs aborted",
	"timeout",
	"file locked",
	"out of memory",
	"too many open files",
	"invalid parameter"
};

static void init(void)
{
	if (!inited)
	{
		FRESULT r;

		mmc_init();
		r = f_mount(0, &fatfs);

		inited = 1;
	}
}

void vfs_sd_deinit(void)
{
	if (inited)
	{
		printf("[unmounting SD card]\n");
		f_mount(0, NULL);
		inited = 0;
	}
}

static void malformed(void)
{
	setError("malformed mem: path (use forward slashes)");
}

static void* open_cb(const char* path, int flags)
{
	FIL* fp = calloc(1, sizeof(FIL));
    FRESULT r;

    init();
    r = f_open(fp, path,
        (flags == O_RDONLY) ? (FA_READ|FA_OPEN_EXISTING) : (FA_WRITE|FA_CREATE_ALWAYS));

	if (r == FR_OK)
		return fp;

	setError("file system error %d: %s", r, error_strings[r]);
	free(fp);
	return NULL;
}

static void close_cb(void* backend)
{
	FIL* fp = backend;
    FRESULT r = f_close(fp);
    free(fp);
    if (r != FR_OK)
		setError("file system error %d: %s", r, error_strings[r]);
}

static uint32_t read_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length)
{
	FIL* fp = backend;
	UINT br;
	FRESULT r = f_lseek(fp, offset);
	if (r != FR_OK)
	{
		setError("file system error %d: %s", r, error_strings[r]);
		return 0;
	}

	r = f_read(fp, buffer, length, &br);
    if (r != FR_OK)
    {
		setError("file system error %d: %s", r, error_strings[r]);
        return 0;
    }

	return br;
}

static uint32_t write_cb(void* backend,
		uint32_t offset, void* buffer, uint32_t length)
{
	FIL* fp = backend;
	UINT br;
	FRESULT r = f_lseek(fp, offset);
	if (r != FR_OK)
	{
		setError("file system error %d: %s", r, error_strings[r]);
		return 0;
	}

	r = f_write(fp, buffer, length, &br);
    if (r != FR_OK)
    {
		setError("file system error %d: %s", r, error_strings[r]);
        return 0;
    }

	return br;
}

static void info_cb(void* backend,
		uint32_t* base, uint32_t* length)
{
	FIL* fp = backend;

	*base = 0;
	*length = f_size(fp);
}

